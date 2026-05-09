#include "SmartHighlight.h"

#include <QByteArray>
#include <QSettings>
#include <QTimer>

#include <Scintilla.h>

namespace {

// Aliases para mensagens Scintilla — manter os macros explícitos
// deixa mais simples cruzar com a documentação oficial.
constexpr int kSciSetIndicatorCurrent = SCI_SETINDICATORCURRENT;
constexpr int kSciIndicatorFillRange  = SCI_INDICATORFILLRANGE;
constexpr int kSciIndicatorClearRange = SCI_INDICATORCLEARRANGE;
constexpr int kSciIndicSetStyle       = SCI_INDICSETSTYLE;
constexpr int kSciIndicSetFore        = SCI_INDICSETFORE;
constexpr int kSciIndicSetAlpha       = SCI_INDICSETALPHA;
constexpr int kSciIndicSetOutAlpha    = SCI_INDICSETOUTLINEALPHA;
constexpr int kSciIndicSetUnder       = SCI_INDICSETUNDER;

constexpr int kIndicStyleFullBox      = INDIC_FULLBOX;     // 16

// Limites do tamanho de palavra elegível para destaque.
constexpr int kMinWordLength = 3;
constexpr int kMaxWordLength = 50;

constexpr int kDebounceMs    = 250;

const char* kOrgName  = "clip52";
const char* kAppName  = "notepadpp-qt";
const char* kKeyEnabled    = "SmartHighlight/Enabled";
const char* kKeyMatchCase  = "SmartHighlight/MatchCase";
const char* kKeyWholeWord  = "SmartHighlight/WholeWord";
const char* kKeyColor      = "SmartHighlight/Color";

} // namespace

SmartHighlight::SmartHighlight(QObject* parent) : QObject(parent) {
    loadSettings();
}

SmartHighlight::~SmartHighlight() = default;

SmartHighlight& SmartHighlight::shared() {
    static SmartHighlight instance;
    return instance;
}

void SmartHighlight::installFor(ScintillaEdit* editor) {
    shared().attach(editor);
}

void SmartHighlight::loadSettings() {
    QSettings s(kOrgName, kAppName);
    m_enabled   = s.value(kKeyEnabled,   true).toBool();
    m_matchCase = s.value(kKeyMatchCase, true).toBool();
    m_wholeWord = s.value(kKeyWholeWord, true).toBool();
    // Cor armazenada como int (formato Scintilla 0x00BBGGRR).
    m_color     = static_cast<sptr_t>(
        s.value(kKeyColor, static_cast<qulonglong>(0x0000FFFF)).toULongLong());
}

void SmartHighlight::saveBool(const char* key, bool value) const {
    QSettings s(kOrgName, kAppName);
    s.setValue(key, value);
}

void SmartHighlight::attach(ScintillaEdit* editor) {
    if (!editor || m_entries.contains(editor)) {
        return;
    }

    EditorEntry entry;
    entry.editor   = editor;
    entry.debounce = new QTimer(this);
    entry.debounce->setSingleShot(true);
    entry.debounce->setInterval(kDebounceMs);
    entry.debounce->setProperty("smarthl_editor",
                                QVariant::fromValue<QObject*>(editor));
    connect(entry.debounce, &QTimer::timeout,
            this, &SmartHighlight::onRescanTimeout);

    m_entries.insert(editor, entry);

    connect(editor, &QObject::destroyed,
            this, &SmartHighlight::onEditorDestroyed);

    // updateUi cobre tanto movimentação de caret quanto mudança de
    // seleção — é o gatilho ideal para "smart highlight".
    connect(editor, &ScintillaEditBase::updateUi,
            this, &SmartHighlight::onUpdateUi);

    if (m_enabled) {
        configureIndicator(editor);
        // Scan inicial para já marcar a palavra sob o caret atual.
        scheduleRescan(editor);
    }
}

void SmartHighlight::detach(ScintillaEdit* editor) {
    if (!editor) return;
    auto it = m_entries.find(editor);
    if (it == m_entries.end()) return;

    disconnect(editor, nullptr, this, nullptr);

    clearIndicators(editor);

    if (it->debounce) {
        it->debounce->stop();
        it->debounce->deleteLater();
    }
    m_entries.erase(it);
}

void SmartHighlight::setEnabled(bool on) {
    if (m_enabled == on) return;
    m_enabled = on;
    saveBool(kKeyEnabled, on);

    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        ScintillaEdit* editor = it.key();
        if (!editor) continue;
        if (m_enabled) {
            configureIndicator(editor);
            it->lastWord.clear();   // força rescan
            scheduleRescan(editor);
        } else {
            clearIndicators(editor);
            it->lastWord.clear();
        }
    }
}

void SmartHighlight::setMatchCase(bool on) {
    if (m_matchCase == on) return;
    m_matchCase = on;
    saveBool(kKeyMatchCase, on);
    // Invalida cache e força rescan em todos os editores.
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        it->lastWord.clear();
        if (m_enabled && it.key()) scheduleRescan(it.key());
    }
}

void SmartHighlight::setWholeWord(bool on) {
    if (m_wholeWord == on) return;
    m_wholeWord = on;
    saveBool(kKeyWholeWord, on);
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        it->lastWord.clear();
        if (m_enabled && it.key()) scheduleRescan(it.key());
    }
}

void SmartHighlight::onEditorDestroyed(QObject* obj) {
    auto* editor = static_cast<ScintillaEdit*>(obj);
    auto it = m_entries.find(editor);
    if (it == m_entries.end()) return;
    if (it->debounce) {
        it->debounce->stop();
        it->debounce->deleteLater();
    }
    m_entries.erase(it);
}

void SmartHighlight::onUpdateUi(Scintilla::Update updated) {
    if (!m_enabled) return;

    // Filtra apenas eventos relevantes: mudança de seleção ou conteúdo.
    using U = Scintilla::Update;
    const auto v = static_cast<int>(updated);
    const int mask = static_cast<int>(U::Selection) | static_cast<int>(U::Content);
    if ((v & mask) == 0) return;

    auto* editor = qobject_cast<ScintillaEdit*>(sender());
    if (!editor) return;
    scheduleRescan(editor);
}

void SmartHighlight::onRescanTimeout() {
    auto* timer = qobject_cast<QTimer*>(sender());
    if (!timer) return;
    auto* obj = timer->property("smarthl_editor").value<QObject*>();
    auto* editor = qobject_cast<ScintillaEdit*>(obj);
    if (editor && m_enabled) {
        rescan(editor);
    }
}

void SmartHighlight::configureIndicator(ScintillaEdit* editor) const {
    if (!editor) return;
    // INDIC_FULLBOX preenche por trás do texto, ideal para destaque
    // tipo "all-occurrences" sem ofuscar a leitura.
    editor->send(kSciIndicSetStyle,    kIndicator, kIndicStyleFullBox);
    editor->send(kSciIndicSetFore,     kIndicator, m_color);
    editor->send(kSciIndicSetAlpha,    kIndicator, m_alpha);
    editor->send(kSciIndicSetOutAlpha, kIndicator, m_outline);
    // Renderiza por baixo do texto (under=1) para não cobri-lo.
    editor->send(kSciIndicSetUnder,    kIndicator, 1);
}

void SmartHighlight::clearIndicators(ScintillaEdit* editor) const {
    if (!editor) return;
    const sptr_t length = editor->textLength();
    if (length <= 0) return;
    editor->send(kSciSetIndicatorCurrent, kIndicator);
    editor->send(kSciIndicatorClearRange, 0, length);
}

void SmartHighlight::scheduleRescan(ScintillaEdit* editor) {
    if (!m_enabled || !editor) return;
    auto it = m_entries.find(editor);
    if (it == m_entries.end() || !it->debounce) return;
    it->debounce->start();
}

QByteArray SmartHighlight::currentTarget(ScintillaEdit* editor) const {
    if (!editor) return {};

    const sptr_t selStart = editor->selectionStart();
    const sptr_t selEnd   = editor->selectionEnd();

    QByteArray target;

    if (selEnd > selStart) {
        // Há seleção explícita — usa-a como alvo. Skippa se
        // multilinha (não faz sentido como "palavra").
        QByteArray sel = editor->getSelText();
        if (sel.endsWith('\0')) sel.chop(1);
        if (sel.contains('\n') || sel.contains('\r')) return {};
        target = sel;
    } else {
        // Sem seleção: extrai a palavra sob o caret.
        const sptr_t caret = editor->currentPos();
        const sptr_t wStart = editor->wordStartPosition(caret, true);
        const sptr_t wEnd   = editor->wordEndPosition(caret, true);
        if (wEnd <= wStart) return {};
        target = editor->textRange(static_cast<int>(wStart),
                                   static_cast<int>(wEnd));
    }

    if (target.isEmpty()) return {};

    // Limites de comprimento (em bytes — UTF-8). Para palavras ASCII
    // isto coincide com o número de caracteres; para o caso multibyte
    // a heurística ainda evita destaques absurdamente curtos/longos.
    const int len = target.size();
    if (len < kMinWordLength || len > kMaxWordLength) return {};

    return target;
}

void SmartHighlight::rescan(ScintillaEdit* editor) {
    if (!editor) return;

    const QByteArray target = currentTarget(editor);
    auto it = m_entries.find(editor);
    if (it == m_entries.end()) return;

    // Cache: se o alvo é igual ao da última varredura, nada a fazer.
    if (target == it->lastWord && !target.isEmpty()) return;
    it->lastWord = target;

    // Sempre limpa antes — inclusive quando não há alvo, para apagar
    // marcações antigas (ex.: caret saiu da palavra).
    clearIndicators(editor);
    if (target.isEmpty()) return;

    // Monta os flags de busca conforme as preferências do usuário.
    sptr_t flags = 0;
    if (m_matchCase) flags |= SCFIND_MATCHCASE;
    if (m_wholeWord) flags |= SCFIND_WHOLEWORD;
    editor->setSearchFlags(flags);

    editor->send(kSciSetIndicatorCurrent, kIndicator);

    const sptr_t docLen = editor->textLength();
    sptr_t cursor = 0;
    const sptr_t targetLen = static_cast<sptr_t>(target.size());

    // Loop clássico SCI_SEARCHINTARGET: a cada hit, marca o range
    // e avança o cursor para depois do match.
    while (cursor < docLen) {
        editor->setTargetRange(cursor, docLen);
        const sptr_t found = editor->searchInTarget(targetLen, target.constData());
        if (found < 0) break;

        const sptr_t mStart = editor->targetStart();
        const sptr_t mEnd   = editor->targetEnd();
        if (mEnd <= mStart) break;

        editor->send(kSciIndicatorFillRange, mStart, mEnd - mStart);
        cursor = mEnd;
    }
}
