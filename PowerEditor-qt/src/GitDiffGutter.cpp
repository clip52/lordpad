#include "GitDiffGutter.h"

#include "ScintillaEdit.h"

#include <QProcess>
#include <QTimer>
#include <QFileInfo>
#include <QDir>
#include <QSettings>
#include <QStringList>
#include <QRegularExpression>

namespace {
// Margin dedicada pra evitar conflito com bookmarks (margin 1) e linha/fold.
constexpr int kDiffMargin = 4;

// Marker IDs reservados pra este módulo.
constexpr int kMarkerAdded    = 20;
constexpr int kMarkerModified = 21;
constexpr int kMarkerRemoved  = 22;
constexpr int kDiffMarkerMask = (1 << kMarkerAdded)
                              | (1 << kMarkerModified)
                              | (1 << kMarkerRemoved);

// Largura da margin em pixels. 4px gera uma faixa fina, suficiente pra
// SC_MARK_FULLRECT/LEFTRECT serem visíveis sem roubar espaço.
constexpr int kMarginWidthPx = 4;

// Cores (formato 0x00BBGGRR — Scintilla usa BGR, não RGB).
constexpr long kColorAdded    = 0x43A02E; // #2ea043 verde
constexpr long kColorModified = 0x2299D2; // #d29922 laranja
constexpr long kColorRemoved  = 0x4951F8; // #f85149 vermelho

// Tempos.
constexpr int kDebounceMsec = 1000;
constexpr int kProcessTimeoutMsec = 2500;

const QString kGitProgram = QStringLiteral("git");
const QString kSettingsKey = QStringLiteral("GitDiffGutter/Enabled");
} // namespace

GitDiffGutter::GitDiffGutter(QObject* parent)
    : QObject(parent)
{
    QSettings s;
    m_enabled = s.value(kSettingsKey, true).toBool();
}

GitDiffGutter::~GitDiffGutter()
{
    // Mata queries em voo e libera recursos.
    for (PendingQuery* q : m_inFlight) {
        if (q->proc) {
            q->proc->disconnect(this);
            q->proc->kill();
            q->proc->waitForFinished(100);
            q->proc->deleteLater();
        }
        if (q->timeout) {
            q->timeout->stop();
            q->timeout->deleteLater();
        }
        delete q;
    }
    m_inFlight.clear();

    for (Binding* b : m_bindings) {
        if (b->debounce) {
            b->debounce->stop();
            b->debounce->deleteLater();
        }
        delete b;
    }
    m_bindings.clear();
}

void GitDiffGutter::attach(ScintillaEdit* editor, const QString& filePath)
{
    if (!editor) return;

    // Se já existe binding pra este editor, apenas atualiza o path.
    if (Binding* existing = findBinding(editor)) {
        existing->filePath = filePath;
        configureEditor(editor);
        if (m_enabled && !filePath.isEmpty()) scheduleQuery(filePath);
        return;
    }

    auto* b = new Binding;
    b->editor = editor;
    b->filePath = filePath;

    // Debounce: re-roda git diff 1s após a última modificação no buffer.
    b->debounce = new QTimer(this);
    b->debounce->setSingleShot(true);
    b->debounce->setInterval(kDebounceMsec);
    connect(b->debounce, &QTimer::timeout, this, [this, b]() {
        if (!m_enabled) return;
        if (!b->editor) return;
        if (b->filePath.isEmpty()) return;
        scheduleQuery(b->filePath);
    });

    // Reage a modificações do conteúdo (qualquer insert/delete) re-armando
    // o debounce. O sinal `modified` carrega flags detalhadas, mas pra
    // disparar refresh basta saber que houve mudança no texto.
    connect(editor, &ScintillaEdit::modified, this,
            [this, b](Scintilla::ModificationFlags type,
                      Scintilla::Position /*position*/,
                      Scintilla::Position /*length*/,
                      Scintilla::Position /*linesAdded*/,
                      const QByteArray& /*text*/,
                      Scintilla::Position /*line*/,
                      Scintilla::FoldLevel /*foldNow*/,
                      Scintilla::FoldLevel /*foldPrev*/) {
                using MF = Scintilla::ModificationFlags;
                const auto mask = MF::InsertText | MF::DeleteText;
                if ((type & mask) == MF::None) return;
                if (b->debounce) b->debounce->start();
            });

    // Garante cleanup se o editor for destruído antes do detach explícito.
    connect(editor, &QObject::destroyed, this, [this, editor]() {
        for (auto it = m_bindings.begin(); it != m_bindings.end(); ++it) {
            if ((*it)->editor.data() == editor) {
                if ((*it)->debounce) {
                    (*it)->debounce->stop();
                    (*it)->debounce->deleteLater();
                }
                delete *it;
                m_bindings.erase(it);
                return;
            }
        }
    });

    m_bindings.append(b);
    configureEditor(editor);

    if (m_enabled && !filePath.isEmpty()) scheduleQuery(filePath);
}

void GitDiffGutter::detach(ScintillaEdit* editor)
{
    if (!editor) return;
    for (auto it = m_bindings.begin(); it != m_bindings.end(); ++it) {
        if ((*it)->editor.data() == editor) {
            disconnect(editor, nullptr, this, nullptr);
            clearMarkers(editor);
            // Esconde a margin pra não deixar uma faixa vazia visível.
            editor->setMarginWidthN(kDiffMargin, 0);
            if ((*it)->debounce) {
                (*it)->debounce->stop();
                (*it)->debounce->deleteLater();
            }
            delete *it;
            m_bindings.erase(it);
            return;
        }
    }
}

void GitDiffGutter::refresh(ScintillaEdit* editor, const QString& filePath)
{
    if (!m_enabled) return;
    if (!editor || filePath.isEmpty()) return;

    // Atualiza o filePath do binding (caso tenha mudado por save-as).
    if (Binding* b = findBinding(editor)) {
        b->filePath = filePath;
    }
    scheduleQuery(filePath);
}

void GitDiffGutter::setEnabled(bool on)
{
    if (m_enabled == on) return;
    m_enabled = on;
    QSettings s;
    s.setValue(kSettingsKey, on);

    if (!on) {
        // Limpa markers de todos os editores attached.
        for (Binding* b : m_bindings) {
            if (b->editor) {
                clearMarkers(b->editor);
                b->editor->setMarginWidthN(kDiffMargin, 0);
            }
            if (b->debounce) b->debounce->stop();
        }
    } else {
        // Reativa: reconfigura margins e re-roda diff em todos.
        for (Binding* b : m_bindings) {
            if (b->editor) configureEditor(b->editor);
            if (!b->filePath.isEmpty()) scheduleQuery(b->filePath);
        }
    }
}

bool GitDiffGutter::isEnabled() const
{
    return m_enabled;
}

void GitDiffGutter::configureEditor(ScintillaEdit* editor)
{
    if (!editor) return;

    // Margin 4: tipo SYMBOL, mostrando apenas os 3 markers deste módulo.
    editor->setMarginTypeN(kDiffMargin, SC_MARGIN_SYMBOL);
    editor->setMarginMaskN(kDiffMargin, kDiffMarkerMask);
    editor->setMarginWidthN(kDiffMargin, m_enabled ? kMarginWidthPx : 0);
    editor->setMarginSensitiveN(kDiffMargin, false);

    // Definição visual dos 3 markers.
    editor->markerDefine(kMarkerAdded,    SC_MARK_FULLRECT);
    editor->markerSetBack(kMarkerAdded,   kColorAdded);
    editor->markerSetFore(kMarkerAdded,   kColorAdded);

    editor->markerDefine(kMarkerModified, SC_MARK_FULLRECT);
    editor->markerSetBack(kMarkerModified, kColorModified);
    editor->markerSetFore(kMarkerModified, kColorModified);

    editor->markerDefine(kMarkerRemoved,  SC_MARK_LEFTRECT);
    editor->markerSetBack(kMarkerRemoved, kColorRemoved);
    editor->markerSetFore(kMarkerRemoved, kColorRemoved);
}

void GitDiffGutter::clearMarkers(ScintillaEdit* editor)
{
    if (!editor) return;
    editor->markerDeleteAll(kMarkerAdded);
    editor->markerDeleteAll(kMarkerModified);
    editor->markerDeleteAll(kMarkerRemoved);
}

void GitDiffGutter::applyMarks(ScintillaEdit* editor, const QList<LineMark>& marks)
{
    if (!editor) return;
    clearMarkers(editor);
    for (const LineMark& m : marks) {
        int marker = kMarkerAdded;
        switch (m.kind) {
        case HunkKind::Added:        marker = kMarkerAdded; break;
        case HunkKind::Modified:     marker = kMarkerModified; break;
        case HunkKind::RemovedAbove: marker = kMarkerRemoved; break;
        }
        editor->markerAdd(m.line, marker);
    }
}

void GitDiffGutter::scheduleQuery(const QString& filePath)
{
    if (filePath.isEmpty()) return;

    // Coalescência: se já há query pra mesmo path, ignora.
    if (m_inFlight.contains(filePath)) return;

    auto* q = new PendingQuery;
    q->filePath = filePath;
    QFileInfo fi(filePath);
    q->workingDir = fi.absolutePath();
    if (q->workingDir.isEmpty() || !QFileInfo(q->workingDir).isDir()) {
        q->workingDir = QDir::currentPath();
    }
    m_inFlight.insert(filePath, q);
    startStage(q);
}

void GitDiffGutter::startStage(PendingQuery* q)
{
    QStringList args;
    QString cwd;

    switch (q->stage) {
    case 0:
        // Etapa 1: descobrir repo root.
        cwd = q->workingDir;
        args << QStringLiteral("-C") << q->workingDir
             << QStringLiteral("rev-parse") << QStringLiteral("--show-toplevel");
        break;
    case 1:
        // Etapa 2: rodar diff de fato com unified=0 pra parsing simples.
        cwd = q->repoRoot;
        args << QStringLiteral("-C") << q->repoRoot
             << QStringLiteral("diff") << QStringLiteral("--no-color")
             << QStringLiteral("--unified=0") << QStringLiteral("HEAD")
             << QStringLiteral("--") << q->relPath;
        break;
    default:
        finalizeWith(q, {});
        return;
    }

    q->proc = new QProcess(this);
    q->proc->setProgram(kGitProgram);
    q->proc->setArguments(args);
    q->proc->setProcessChannelMode(QProcess::SeparateChannels);
    q->proc->setWorkingDirectory(cwd);

    connect(q->proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, q](int code, QProcess::ExitStatus st) {
                onProcessFinished(q, code, static_cast<int>(st));
            });
    connect(q->proc, &QProcess::errorOccurred, this,
            [this, q](QProcess::ProcessError err) {
                if (err == QProcess::FailedToStart) finalizeWith(q, {});
            });

    q->timeout = new QTimer(this);
    q->timeout->setSingleShot(true);
    q->timeout->setInterval(kProcessTimeoutMsec);
    connect(q->timeout, &QTimer::timeout, this, [this, q]() { onTimeout(q); });
    q->timeout->start();

    q->proc->start();
}

void GitDiffGutter::onProcessFinished(PendingQuery* q, int exitCode, int exitStatus)
{
    if (!q || !q->proc) return;

    if (q->timeout) {
        q->timeout->stop();
        q->timeout->deleteLater();
        q->timeout = nullptr;
    }

    const QString stdoutStr = QString::fromUtf8(q->proc->readAllStandardOutput());
    q->proc->deleteLater();
    q->proc = nullptr;

    const bool ok = (exitStatus == static_cast<int>(QProcess::NormalExit) && exitCode == 0);

    switch (q->stage) {
    case 0: {
        // rev-parse falhou -> não é repo: limpa markers e finaliza.
        if (!ok || stdoutStr.trimmed().isEmpty()) {
            finalizeWith(q, {});
            return;
        }
        q->repoRoot = stdoutStr.trimmed();
        q->relPath = QDir(q->repoRoot).relativeFilePath(q->filePath);
        q->stage = 1;
        startStage(q);
        return;
    }
    case 1: {
        // Diff exit code: 0 = sem mudanças, 1 = com mudanças. Ambos válidos.
        // Outros códigos (>=128) indicam erro (path inexistente etc.).
        const bool diffOk = (exitStatus == static_cast<int>(QProcess::NormalExit)
                              && exitCode <= 1);
        if (!diffOk) {
            finalizeWith(q, {});
            return;
        }
        const QList<LineMark> marks = parseDiff(stdoutStr);
        finalizeWith(q, marks);
        return;
    }
    }
}

void GitDiffGutter::onTimeout(PendingQuery* q)
{
    if (!q) return;

    if (q->proc) {
        q->proc->disconnect(this);
        q->proc->kill();
        q->proc->waitForFinished(100);
        q->proc->deleteLater();
        q->proc = nullptr;
    }
    if (q->timeout) {
        q->timeout->deleteLater();
        q->timeout = nullptr;
    }
    finalizeWith(q, {});
}

void GitDiffGutter::finalizeWith(PendingQuery* q, const QList<LineMark>& marks)
{
    if (!q) return;

    const QString path = q->filePath;
    m_inFlight.remove(path);

    if (q->proc) {
        q->proc->disconnect(this);
        q->proc->deleteLater();
    }
    if (q->timeout) {
        q->timeout->stop();
        q->timeout->deleteLater();
    }
    delete q;

    if (!m_enabled) return;

    // Aplica em todos os editores attached cujo filePath bate (pode haver
    // múltiplas views do mesmo arquivo em split-view).
    for (Binding* b : m_bindings) {
        if (!b->editor) continue;
        if (b->filePath != path) continue;
        applyMarks(b->editor, marks);
    }
}

QList<GitDiffGutter::LineMark> GitDiffGutter::parseDiff(const QString& diffOutput)
{
    QList<LineMark> result;
    if (diffOutput.isEmpty()) return result;

    // Hunk header: @@ -oldStart[,oldCount] +newStart[,newCount] @@
    // Note: oldCount/newCount são opcionais (default 1). unified=0 sempre
    // emite um hunk por bloco contíguo, então parseamos cada um.
    static const QRegularExpression kHunkRe(
        QStringLiteral("^@@ -(\\d+)(?:,(\\d+))? \\+(\\d+)(?:,(\\d+))? @@"));

    const QStringList lines = diffOutput.split(QChar('\n'));

    int  newStart = 0;
    int  newCount = 0;
    int  oldCount = 0;
    bool inHunk   = false;
    bool hasPlus  = false;
    bool hasMinus = false;
    int  curLine  = 0;       // próxima linha do "lado novo" a marcar

    auto flushHunk = [&]() {
        if (!inHunk) return;
        // Hunk com newCount==0 -> deleção pura: marca a linha newStart
        // (que no diff unified=0 é a linha *após* a deleção, ou a última
        // linha do arquivo quando deletamos no fim). 1-based -> 0-based:
        // se newStart == 0 (arquivo vazio após), usamos linha 0.
        if (newCount == 0) {
            const int line0 = (newStart > 0) ? (newStart - 1) : 0;
            result.append({ line0, HunkKind::RemovedAbove });
        }
        inHunk = false;
    };

    for (const QString& raw : lines) {
        if (raw.startsWith(QStringLiteral("@@"))) {
            // Fecha hunk anterior antes de abrir o novo.
            flushHunk();

            const auto m = kHunkRe.match(raw);
            if (!m.hasMatch()) {
                inHunk = false;
                continue;
            }
            // Captura grupos: 1=oldStart, 2=oldCount?, 3=newStart, 4=newCount?
            oldCount = m.captured(2).isEmpty() ? 1 : m.captured(2).toInt();
            newStart = m.captured(3).toInt();
            newCount = m.captured(4).isEmpty() ? 1 : m.captured(4).toInt();
            inHunk   = true;
            hasPlus  = false;
            hasMinus = (oldCount > 0);
            curLine  = newStart;  // 1-based
            continue;
        }

        if (!inHunk) continue;

        // Pula headers de arquivo (+++ /path ou --- /path) que aparecem
        // antes do primeiro @@. Já tratado pelo `inHunk == false` acima.
        if (raw.startsWith(QStringLiteral("+++")) ||
            raw.startsWith(QStringLiteral("---"))) {
            continue;
        }

        if (raw.startsWith(QChar('+'))) {
            hasPlus = true;
            // Adicionada/modificada: marca a linha 1-based curLine.
            // Decisão added vs modified depende de hasMinus do hunk inteiro,
            // então registramos "Added" provisoriamente e ajustamos abaixo.
            const int line0 = (curLine > 0) ? (curLine - 1) : 0;
            result.append({ line0, hasMinus ? HunkKind::Modified
                                            : HunkKind::Added });
            ++curLine;
            continue;
        }
        if (raw.startsWith(QChar('-'))) {
            // Linha removida: não consome curLine (não existe no lado novo).
            continue;
        }
        // Linha de contexto (não deveria aparecer com unified=0, mas se
        // aparecer, avança o cursor do lado novo).
        if (!raw.isEmpty() && raw.at(0) == QChar(' ')) {
            ++curLine;
        }
    }
    // Fecha o último hunk (caso seja deleção pura).
    flushHunk();

    // Pós-processamento: se dentro do mesmo hunk houve `+` E `-`, todas
    // as `+` daquele hunk devem ser Modified, não Added. Como aplicamos
    // o flag `hasMinus` no momento do `+`, a marca já fica correta porque
    // hasMinus é true sempre que oldCount > 0 — ou seja, o hunk teve `-`
    // mesmo que ainda não tenhamos visto a linha `-` (em unified=0 elas
    // sempre vêm antes das `+` do mesmo hunk). Nada mais a fazer.

    return result;
}

GitDiffGutter::Binding* GitDiffGutter::findBinding(ScintillaEdit* editor)
{
    for (Binding* b : m_bindings) {
        if (b->editor.data() == editor) return b;
    }
    return nullptr;
}

GitDiffGutter::Binding* GitDiffGutter::findBindingByPath(const QString& path)
{
    for (Binding* b : m_bindings) {
        if (b->filePath == path) return b;
    }
    return nullptr;
}
