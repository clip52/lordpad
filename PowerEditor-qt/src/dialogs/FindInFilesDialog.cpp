#include "FindInFilesDialog.h"

#include "../FileIO.h"
#include "../SearchEscapes.h"

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QCheckBox>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QFileDialog>
#include <QRegularExpression>
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include <QElapsedTimer>
#include <QHeaderView>
#include <QTimer>
#include <QStringConverter>
#include <QByteArray>
#include <QMetaObject>
#include <QMessageBox>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QProcess>
#include <QStandardPaths>
#include <QList>
#include <QString>
#include <QStringList>
#include <atomic>
#include <memory>

namespace {
constexpr qint64 kMaxFileSize    = 5 * 1024 * 1024;   // 5 MiB
constexpr int    kBinarySniffLen = 8 * 1024;          // 8 KiB
constexpr double kBinaryNullPct  = 0.05;              // > 5% null bytes => binary
constexpr int    kBatchSize      = 50;                // hits per flush
constexpr int    kBatchIntervalMs = 100;              // ms between forced flushes
constexpr int    kLineTextCap    = 200;               // chars
} // namespace

FindInFilesDialog::FindInFilesDialog(QWidget* parent)
    : QDialog(parent)
{
    qRegisterMetaType<FindInFilesDialog::Hit>("FindInFilesDialog::Hit");
    qRegisterMetaType<QList<FindInFilesDialog::Hit>>("QList<FindInFilesDialog::Hit>");

    setWindowTitle(tr("Find in Files"));
    resize(1000, 650);

    m_cancel        = std::make_shared<std::atomic<bool>>(false);
    m_filesScanned  = std::make_shared<std::atomic<int>>(0);
    m_matchesFound  = std::make_shared<std::atomic<int>>(0);

    setupUi();
}

FindInFilesDialog::~FindInFilesDialog()
{
    if (m_cancel) m_cancel->store(true);
    if (m_watcher) {
        m_watcher->waitForFinished();
    }
}

void FindInFilesDialog::setupUi()
{
    auto* root = new QVBoxLayout(this);

    // ------- top form -------
    auto* form = new QFormLayout();

    m_pattern = new QLineEdit(this);
    form->addRow(tr("Find what:"), m_pattern);

    auto* folderRow = new QHBoxLayout();
    m_folder = new QLineEdit(this);
    m_browseBtn = new QToolButton(this);
    m_browseBtn->setText(tr("Browse..."));
    folderRow->addWidget(m_folder, 1);
    folderRow->addWidget(m_browseBtn);
    form->addRow(tr("In folder:"), folderRow);

    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText(QStringLiteral("*.cpp;*.h;*.py"));
    form->addRow(tr("File filter:"), m_filter);

    root->addLayout(form);

    // ------- options row -------
    auto* opts = new QHBoxLayout();
    m_caseChk    = new QCheckBox(tr("Match case"), this);
    m_wordChk    = new QCheckBox(tr("Whole word"), this);
    m_regexChk   = new QCheckBox(tr("Regular expression"), this);
    m_hiddenChk  = new QCheckBox(tr("Include hidden"), this);
    m_symlinkChk = new QCheckBox(tr("Follow symlinks"), this);
    m_useRgChk   = new QCheckBox(tr("Use ripgrep (rg)"), this);   // M11
    m_extendedChk = new QCheckBox(tr("Extended (\\n, \\t, …)"), this);   // M12
    m_hiddenChk->setChecked(false);
    m_symlinkChk->setChecked(false);
    {
        // Default: on iff `rg` is on PATH. Disable + tooltip otherwise so the
        // user knows why ripgrep mode isn't kicking in.
        const QString rgPath = QStandardPaths::findExecutable(QStringLiteral("rg"));
        m_useRgChk->setChecked(!rgPath.isEmpty());
        if (rgPath.isEmpty()) {
            m_useRgChk->setEnabled(false);
            m_useRgChk->setToolTip(tr("`rg` (ripgrep) não foi encontrado no PATH."));
        } else {
            m_useRgChk->setToolTip(tr("Usa ripgrep — tipicamente 10–50× mais rápido."));
        }
    }
    opts->addWidget(m_caseChk);
    opts->addWidget(m_wordChk);
    opts->addWidget(m_regexChk);
    opts->addWidget(m_hiddenChk);
    opts->addWidget(m_symlinkChk);
    opts->addWidget(m_useRgChk);
    opts->addWidget(m_extendedChk);
    opts->addStretch(1);
    root->addLayout(opts);

    // Extended and Regex are mutually exclusive — same convention as FindReplaceDialog.
    connect(m_regexChk,    &QCheckBox::toggled, this, [this](bool on) {
        if (on) m_extendedChk->setChecked(false);
    });
    connect(m_extendedChk, &QCheckBox::toggled, this, [this](bool on) {
        if (on) m_regexChk->setChecked(false);
    });

    // ------- replace row (M12) -------
    auto* replRow = new QHBoxLayout();
    auto* replLabel = new QLabel(tr("Replace with:"), this);
    m_replaceEdit = new QLineEdit(this);
    m_replaceEdit->setPlaceholderText(tr("(opcional) substituição — Replace All percorre todos os hits"));
    m_replaceAllBtn = new QPushButton(tr("Replace All"), this);
    replRow->addWidget(replLabel);
    replRow->addWidget(m_replaceEdit, 1);
    replRow->addWidget(m_replaceAllBtn);
    root->addLayout(replRow);

    // ------- buttons row -------
    auto* btns = new QHBoxLayout();
    m_searchBtn = new QPushButton(tr("Search"), this);
    m_stopBtn   = new QPushButton(tr("Stop"), this);
    m_clearBtn  = new QPushButton(tr("Clear results"), this);
    m_closeBtn  = new QPushButton(tr("Close"), this);
    m_stopBtn->setEnabled(false);
    btns->addWidget(m_searchBtn);
    btns->addWidget(m_stopBtn);
    btns->addWidget(m_clearBtn);
    btns->addStretch(1);
    btns->addWidget(m_closeBtn);
    root->addLayout(btns);

    // ------- result tree -------
    m_tree = new QTreeView(this);
    m_model = new QStandardItemModel(this);
    m_model->setHorizontalHeaderLabels(QStringList()
                                       << tr("File")
                                       << tr("Line")
                                       << tr("Match"));
    m_tree->setModel(m_model);
    m_tree->setUniformRowHeights(true);
    m_tree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tree->header()->setStretchLastSection(true);
    m_tree->setColumnWidth(0, 460);
    m_tree->setColumnWidth(1, 70);
    root->addWidget(m_tree, 1);

    // ------- status -------
    m_status = new QLabel(this);
    m_status->setText(tr("Searched 0 files, found 0 matches in 0 ms"));
    root->addWidget(m_status);

    // signals
    connect(m_browseBtn, &QToolButton::clicked, this, &FindInFilesDialog::onBrowse);
    connect(m_searchBtn, &QPushButton::clicked, this, &FindInFilesDialog::onSearch);
    connect(m_stopBtn,   &QPushButton::clicked, this, &FindInFilesDialog::onStop);
    connect(m_clearBtn,  &QPushButton::clicked, this, &FindInFilesDialog::onClearResults);
    connect(m_closeBtn,  &QPushButton::clicked, this, &QDialog::reject);
    connect(m_tree, &QTreeView::doubleClicked, this, &FindInFilesDialog::onResultActivated);
    connect(m_pattern, &QLineEdit::returnPressed, this, &FindInFilesDialog::onSearch);
    connect(m_replaceAllBtn, &QPushButton::clicked, this, &FindInFilesDialog::onReplaceAll);
}

// ---------------- public setters ----------------

void FindInFilesDialog::setSearchFolder(const QString& path)
{
    if (m_folder) m_folder->setText(path);
}

void FindInFilesDialog::setSearchPattern(const QString& pattern)
{
    if (m_pattern) m_pattern->setText(pattern);
}

// ---------------- helpers ----------------

void FindInFilesDialog::setSearchingUi(bool searching)
{
    m_searchBtn->setEnabled(!searching);
    m_stopBtn->setEnabled(searching);
    m_pattern->setEnabled(!searching);
    m_folder->setEnabled(!searching);
    m_browseBtn->setEnabled(!searching);
    m_filter->setEnabled(!searching);
    m_caseChk->setEnabled(!searching);
    m_wordChk->setEnabled(!searching);
    m_regexChk->setEnabled(!searching);
    m_hiddenChk->setEnabled(!searching);
    m_symlinkChk->setEnabled(!searching);
    m_running = searching;
}

void FindInFilesDialog::updateStatus(const QString& extra)
{
    const int files = m_filesScanned ? m_filesScanned->load() : 0;
    const int hits  = m_matchesFound ? m_matchesFound->load() : 0;
    QString base = tr("Searched %1 files, found %2 matches in %3 ms")
                       .arg(files).arg(hits).arg(m_elapsedMs);
    if (!extra.isEmpty())
        base += QStringLiteral(" ") + extra;
    m_status->setText(base);
}

// ---------------- slots ----------------

void FindInFilesDialog::onBrowse()
{
    const QString start = m_folder->text().isEmpty()
                              ? QDir::homePath()
                              : m_folder->text();
    const QString chosen = QFileDialog::getExistingDirectory(
        this, tr("Choose folder"), start);
    if (!chosen.isEmpty())
        m_folder->setText(chosen);
}

void FindInFilesDialog::onClearResults()
{
    m_model->removeRows(0, m_model->rowCount());
    m_fileNodes.clear();
    if (m_filesScanned) m_filesScanned->store(0);
    if (m_matchesFound) m_matchesFound->store(0);
    m_elapsedMs = 0;
    m_pendingHits.clear();
    m_status->setText(tr("Searched 0 files, found 0 matches in 0 ms"));
}

void FindInFilesDialog::onResultActivated(const QModelIndex& index)
{
    if (!index.isValid()) return;
    // child rows have a parent; top-level nodes have an invalid parent
    if (!index.parent().isValid()) {
        // top-level: toggle expand
        if (m_tree->isExpanded(index)) m_tree->collapse(index);
        else                            m_tree->expand(index);
        return;
    }

    // child - find the row's items (line column is sibling 1, file path lives on parent)
    const QModelIndex parentIdx = index.parent();
    QStandardItem* fileItem = m_model->itemFromIndex(parentIdx);
    const QModelIndex lineIdx = index.sibling(index.row(), 1);
    QStandardItem* lineItem = m_model->itemFromIndex(lineIdx);
    if (!fileItem || !lineItem) return;

    const QString filePath = fileItem->data(Qt::UserRole).toString();
    bool ok = false;
    const int line = lineItem->text().toInt(&ok);
    if (ok && !filePath.isEmpty())
        emit openFileRequested(filePath, line);
}

void FindInFilesDialog::onStop()
{
    if (!m_running) return;
    if (m_cancel) m_cancel->store(true);
    if (m_rgProc) m_rgProc->kill();   // M11: also stop the rg subprocess
    m_stopped = true;
    m_status->setText(m_status->text() + QStringLiteral(" ") + tr("(stopping...)"));
}

void FindInFilesDialog::onSearch()
{
    if (m_running) return;

    QString       pattern = m_pattern->text();
    const QString folder  = m_folder->text();
    const QString filter  = m_filter->text();

    // M12: Extended escapes are expanded once at the entry point so both
    // the QtConcurrent worker and the rg subprocess see the same literal text.
    if (m_extendedChk && m_extendedChk->isChecked())
        pattern = SearchEscapes::expandExtended(pattern);

    if (pattern.isEmpty()) {
        m_status->setText(tr("Pattern is empty."));
        return;
    }
    if (folder.isEmpty() || !QFileInfo(folder).isDir()) {
        m_status->setText(tr("Folder does not exist."));
        return;
    }

    // validate regex up front
    if (m_regexChk->isChecked()) {
        QRegularExpression rx(pattern,
                              m_caseChk->isChecked()
                                  ? QRegularExpression::NoPatternOption
                                  : QRegularExpression::CaseInsensitiveOption);
        if (!rx.isValid()) {
            m_status->setText(tr("Invalid regular expression: %1").arg(rx.errorString()));
            return;
        }
    }

    onClearResults();

    m_rootFolder = folder;
    m_stopped = false;
    if (m_cancel) m_cancel->store(false);
    if (m_filesScanned) m_filesScanned->store(0);
    if (m_matchesFound) m_matchesFound->store(0);
    m_elapsedMs = 0;

    setSearchingUi(true);
    m_status->setText(tr("(searching...)"));

    const bool matchCase     = m_caseChk->isChecked();
    const bool wholeWord     = m_wordChk->isChecked();
    const bool regex         = m_regexChk->isChecked();
    const bool includeHidden = m_hiddenChk->isChecked();
    const bool followSym     = m_symlinkChk->isChecked();

    // M11: ripgrep fast path. Fall back to the QtConcurrent worker if rg
    // isn't available or the user disabled the checkbox.
    if (m_useRgChk && m_useRgChk->isChecked()
        && startRipgrep(folder, pattern, filter, matchCase, wholeWord,
                        regex, includeHidden, followSym)) {
        return;
    }

    if (!m_watcher) {
        m_watcher = new QFutureWatcher<void>(this);
        connect(m_watcher, &QFutureWatcher<void>::finished,
                this, &FindInFilesDialog::onSearchFinished);
    }

    // single background thread - no parallel file IO
    auto future = QtConcurrent::run(
        [this, folder, pattern, filter,
         matchCase, wholeWord, regex, includeHidden, followSym]() {
            this->runWorker(folder, pattern, filter,
                            matchCase, wholeWord, regex,
                            includeHidden, followSym);
        });
    m_watcher->setFuture(future);
}

bool FindInFilesDialog::startRipgrep(const QString& folder, const QString& pattern,
                                     const QString& filter, bool matchCase,
                                     bool wholeWord, bool regex,
                                     bool includeHidden, bool followSymlinks)
{
    const QString rgPath = QStandardPaths::findExecutable(QStringLiteral("rg"));
    if (rgPath.isEmpty()) return false;

    QStringList args;
    args << QStringLiteral("--json")
         << QStringLiteral("--no-heading")
         << QStringLiteral("--with-filename")
         << QStringLiteral("--line-number");
    if (!matchCase)     args << QStringLiteral("--ignore-case");
    if (wholeWord)      args << QStringLiteral("--word-regexp");
    if (!regex)         args << QStringLiteral("--fixed-strings");
    if (includeHidden)  args << QStringLiteral("--hidden");
    if (followSymlinks) args << QStringLiteral("--follow");
    if (!filter.isEmpty()) {
        // The dialog accepts comma- or space-separated globs; forward each
        // as `--glob` so rg's gitignore + glob filter applies cleanly.
        const QStringList globs = filter.split(QRegularExpression(QStringLiteral(R"RX([\s,;])RX")),
                                                Qt::SkipEmptyParts);
        for (const QString& g : globs) args << QStringLiteral("--glob") << g;
    }
    args << QStringLiteral("--") << pattern << folder;

    if (m_rgProc) { m_rgProc->kill(); m_rgProc->deleteLater(); m_rgProc = nullptr; }
    m_rgBuf.clear();
    m_rgProc = new QProcess(this);
    connect(m_rgProc, &QProcess::readyReadStandardOutput, this, [this]() {
        m_rgBuf += m_rgProc->readAllStandardOutput();
        int nl;
        while ((nl = m_rgBuf.indexOf('\n')) >= 0) {
            QByteArray line = m_rgBuf.left(nl);
            m_rgBuf.remove(0, nl + 1);
            parseRipgrepLine(line);
        }
    });
    connect(m_rgProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int /*code*/, QProcess::ExitStatus /*st*/) {
                // Drain any tail without trailing newline.
                if (!m_rgBuf.isEmpty()) parseRipgrepLine(m_rgBuf);
                m_rgBuf.clear();
                m_rgProc->deleteLater();
                m_rgProc = nullptr;
                onSearchFinished();
            });
    m_rgProc->start(rgPath, args);
    if (!m_rgProc->waitForStarted(2000)) {
        delete m_rgProc;
        m_rgProc = nullptr;
        return false;
    }
    return true;
}

void FindInFilesDialog::parseRipgrepLine(const QByteArray& line)
{
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(line, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return;
    const QJsonObject obj = doc.object();
    if (obj.value("type").toString() != QStringLiteral("match")) return;

    const QJsonObject data    = obj.value("data").toObject();
    const QString filePath    = data.value("path").toObject().value("text").toString();
    const QString lineText    = data.value("lines").toObject().value("text").toString();
    const int     lineNumber  = data.value("line_number").toInt();
    if (filePath.isEmpty() || lineNumber <= 0) return;

    Hit h;
    h.filePath   = filePath;
    h.lineNumber = lineNumber;
    h.lineText   = lineText.trimmed();
    QList<Hit> one;
    one.append(h);
    receiveHits(one);
}

void FindInFilesDialog::onSearchFinished()
{
    // flush any leftover pending hits (worker should have done it already
    // via the final QueuedConnection invokeMethod, but be safe)
    if (!m_pendingHits.isEmpty()) appendBatch();

    setSearchingUi(false);
    if (m_stopped) {
        updateStatus(tr("(stopped)"));
    } else {
        updateStatus();
    }
}

// ---------------- batch append ----------------

void FindInFilesDialog::appendBatch()
{
    if (m_pendingHits.isEmpty()) return;

    QList<Hit> batch;
    batch.swap(m_pendingHits);

    for (const Hit& hit : batch) {
        QStandardItem* fileNode = m_fileNodes.value(hit.filePath, nullptr);
        if (!fileNode) {
            QString display = hit.filePath;
            if (!m_rootFolder.isEmpty()) {
                const QDir root(m_rootFolder);
                const QString rel = root.relativeFilePath(hit.filePath);
                if (!rel.isEmpty() && !rel.startsWith(QStringLiteral("..")))
                    display = rel;
            }
            fileNode = new QStandardItem(display);
            fileNode->setData(hit.filePath, Qt::UserRole);
            fileNode->setEditable(false);
            QStandardItem* spacer1 = new QStandardItem();
            QStandardItem* spacer2 = new QStandardItem();
            spacer1->setEditable(false);
            spacer2->setEditable(false);
            QList<QStandardItem*> rowItems;
            rowItems << fileNode << spacer1 << spacer2;
            m_model->appendRow(rowItems);
            m_fileNodes.insert(hit.filePath, fileNode);
        }

        QString text = hit.lineText;
        if (text.size() > kLineTextCap)
            text = text.left(kLineTextCap) + QStringLiteral("...");

        auto* col0 = new QStandardItem();
        auto* col1 = new QStandardItem(QString::number(hit.lineNumber));
        auto* col2 = new QStandardItem(text);
        col0->setEditable(false);
        col1->setEditable(false);
        col2->setEditable(false);
        QList<QStandardItem*> childRow;
        childRow << col0 << col1 << col2;
        fileNode->appendRow(childRow);
    }

    updateStatus(m_running ? tr("(searching...)") : QString());
}

void FindInFilesDialog::receiveHits(const QList<FindInFilesDialog::Hit>& hits)
{
    m_pendingHits.append(hits);
    appendBatch();
}

void FindInFilesDialog::receiveError(const QString& message)
{
    m_status->setText(message);
}

// ---------------- worker (background thread) ----------------

void FindInFilesDialog::runWorker(const QString& folder,
                                  const QString& pattern,
                                  const QString& filter,
                                  bool matchCase,
                                  bool wholeWord,
                                  bool regex,
                                  bool includeHidden,
                                  bool followSymlinks)
{
    QElapsedTimer total;
    total.start();
    QElapsedTimer flushTimer;
    flushTimer.start();

    // ---- prepare filter glob list ----
    QList<QRegularExpression> globRxs;
    {
        const QStringList globs = filter.split(QLatin1Char(';'),
                                               Qt::SkipEmptyParts);
        for (QString g : globs) {
            g = g.trimmed();
            if (g.isEmpty()) continue;
            QString rxStr = QRegularExpression::wildcardToRegularExpression(g);
            QRegularExpression rx(rxStr,
                                  QRegularExpression::CaseInsensitiveOption);
            if (rx.isValid()) globRxs.push_back(rx);
        }
    }

    // ---- prepare matcher ----
    QRegularExpression matchRx;
    bool useRegex = false;
    if (regex) {
        QRegularExpression::PatternOptions opts =
            matchCase ? QRegularExpression::NoPatternOption
                      : QRegularExpression::CaseInsensitiveOption;
        matchRx.setPattern(pattern);
        matchRx.setPatternOptions(opts);
        if (!matchRx.isValid()) {
            QMetaObject::invokeMethod(this, "receiveError",
                                      Qt::QueuedConnection,
                                      Q_ARG(QString,
                                            tr("Invalid regular expression: %1")
                                                .arg(matchRx.errorString())));
            return;
        }
        useRegex = true;
    } else if (wholeWord) {
        QRegularExpression::PatternOptions opts =
            matchCase ? QRegularExpression::NoPatternOption
                      : QRegularExpression::CaseInsensitiveOption;
        matchRx.setPattern(QStringLiteral("\\b") +
                           QRegularExpression::escape(pattern) +
                           QStringLiteral("\\b"));
        matchRx.setPatternOptions(opts);
        useRegex = true;
    }

    const Qt::CaseSensitivity cs = matchCase ? Qt::CaseSensitive
                                             : Qt::CaseInsensitive;

    // ---- iterator filters ----
    QDir::Filters dirFilters = QDir::Files | QDir::NoDotAndDotDot;
    if (includeHidden) dirFilters |= QDir::Hidden | QDir::System;
    QDirIterator::IteratorFlags itFlags = QDirIterator::Subdirectories;
    if (followSymlinks) itFlags |= QDirIterator::FollowSymlinks;

    QDirIterator it(folder, QStringList(), dirFilters, itFlags);

    QList<Hit> batch;

    auto flushBatch = [&](bool force) {
        if (batch.isEmpty()) return;
        if (!force && batch.size() < kBatchSize &&
            flushTimer.elapsed() < kBatchIntervalMs) return;
        QMetaObject::invokeMethod(this, "receiveHits",
                                  Qt::QueuedConnection,
                                  Q_ARG(QList<FindInFilesDialog::Hit>, batch));
        batch.clear();
        flushTimer.restart();
    };

    while (it.hasNext()) {
        if (m_cancel && m_cancel->load()) break;
        const QString filePath = it.next();
        const QFileInfo fi = it.fileInfo();
        if (!fi.isFile()) continue;

        // hidden filter (extra safety - QDirIterator already obeys QDir::Hidden)
        if (!includeHidden && fi.isHidden()) continue;

        // glob match
        if (!globRxs.isEmpty()) {
            const QString name = fi.fileName();
            bool any = false;
            for (const auto& rx : globRxs) {
                if (rx.match(name).hasMatch()) { any = true; break; }
            }
            if (!any) continue;
        }

        // size cap
        if (fi.size() > kMaxFileSize) {
            m_filesScanned->fetch_add(1);
            continue;
        }

        QFile f(filePath);
        if (!f.open(QIODevice::ReadOnly)) {
            m_filesScanned->fetch_add(1);
            continue;
        }
        const QByteArray bytes = f.readAll();
        f.close();

        // binary sniff: > 5% null bytes in first 8 KiB
        const int sniff = qMin<int>(bytes.size(), kBinarySniffLen);
        if (sniff > 0) {
            int nulls = 0;
            for (int i = 0; i < sniff; ++i)
                if (bytes[i] == '\0') ++nulls;
            if (static_cast<double>(nulls) / sniff > kBinaryNullPct) {
                m_filesScanned->fetch_add(1);
                continue;
            }
        }

        // decode UTF-8 (replace invalid sequences)
        QStringDecoder dec(QStringConverter::Utf8,
                           QStringDecoder::Flag::ConvertInvalidToNull);
        QString text = dec.decode(bytes);

        m_filesScanned->fetch_add(1);

        // Iterate lines without making N copies of substrings: use indexOf.
        int lineNo = 0;
        int start  = 0;
        const int n = text.size();
        while (start <= n) {
            if (m_cancel && m_cancel->load()) break;
            int nl = text.indexOf(QLatin1Char('\n'), start);
            int end = (nl < 0) ? n : nl;
            // strip trailing \r
            int lineEnd = end;
            if (lineEnd > start && text.at(lineEnd - 1) == QLatin1Char('\r'))
                --lineEnd;
            ++lineNo;

            QStringView line(text.constData() + start, lineEnd - start);

            bool hit = false;
            if (useRegex) {
                if (matchRx.match(line).hasMatch()) hit = true;
            } else {
                if (line.contains(pattern, cs)) hit = true;
            }

            if (hit) {
                Hit h;
                h.filePath   = filePath;
                h.lineNumber = lineNo;
                h.lineText   = line.toString();
                batch.push_back(h);
                m_matchesFound->fetch_add(1);
                flushBatch(false);
            }

            if (nl < 0) break;
            start = nl + 1;
        }

        flushBatch(false);
    }

    // final flush + record elapsed
    QMetaObject::invokeMethod(this, [this, elapsed = total.elapsed()]() {
        m_elapsedMs = elapsed;
    }, Qt::QueuedConnection);
    flushBatch(true);
}

// ---------------- replace-in-files (M12) ----------------

void FindInFilesDialog::onReplaceAll()
{
    if (m_running) {
        QMessageBox::information(this, tr("Replace"),
            tr("Aguarde a busca atual terminar."));
        return;
    }
    if (m_fileNodes.isEmpty()) {
        QMessageBox::information(this, tr("Replace"),
            tr("Faça uma busca primeiro — o Replace percorre os arquivos com hits."));
        return;
    }
    QString pattern     = m_pattern->text();
    QString replacement = m_replaceEdit->text();
    if (pattern.isEmpty()) {
        QMessageBox::warning(this, tr("Replace"), tr("Padrão vazio."));
        return;
    }
    const bool extended  = m_extendedChk && m_extendedChk->isChecked();
    const bool regex     = m_regexChk    && m_regexChk->isChecked();
    const bool matchCase = m_caseChk     && m_caseChk->isChecked();
    const bool wholeWord = m_wordChk     && m_wordChk->isChecked();
    if (extended) {
        pattern     = SearchEscapes::expandExtended(pattern);
        replacement = SearchEscapes::expandExtended(replacement);
    }

    // Build the regex once. For non-regex mode we still use QRegularExpression
    // with literal text (escaped) — it gives us a single code path for whole-word
    // and case-insensitive handling.
    QString rxSource = regex ? pattern : QRegularExpression::escape(pattern);
    if (wholeWord) rxSource = QStringLiteral("\\b") + rxSource + QStringLiteral("\\b");
    QRegularExpression rx(rxSource,
        matchCase ? QRegularExpression::NoPatternOption
                  : QRegularExpression::CaseInsensitiveOption);
    if (!rx.isValid()) {
        QMessageBox::warning(this, tr("Replace"),
            tr("Regex inválida: %1").arg(rx.errorString()));
        return;
    }

    const int fileCount = m_fileNodes.size();
    const auto answer = QMessageBox::question(this, tr("Replace All"),
        tr("Substituir em %1 arquivo(s)?\n\n"
           "Os arquivos serão regravados em UTF-8 — faça commit ou backup antes "
           "se eles tiverem encoding diferente.").arg(fileCount),
        QMessageBox::Yes | QMessageBox::No);
    if (answer != QMessageBox::Yes) return;

    int filesChanged = 0;
    int totalReplacements = 0;
    QStringList failures;
    const QStringList paths = m_fileNodes.keys();
    for (const QString& path : paths) {
        FileIO::LoadResult lr = FileIO::readFile(path);
        if (!lr.ok) { failures << path; continue; }
        QString text = QString::fromUtf8(lr.utf8);

        // Count replacements before applying so we have an honest summary.
        int replacementsHere = 0;
        auto it = rx.globalMatch(text);
        while (it.hasNext()) { it.next(); ++replacementsHere; }
        if (replacementsHere == 0) continue;

        text.replace(rx, replacement);
        QString err;
        if (!FileIO::writeFile(path, text.toUtf8(), &err)) {
            failures << QStringLiteral("%1 — %2").arg(path, err);
            continue;
        }
        ++filesChanged;
        totalReplacements += replacementsHere;
    }

    QString msg = tr("%1 substituição(ões) em %2 arquivo(s).")
                      .arg(totalReplacements).arg(filesChanged);
    if (!failures.isEmpty()) {
        msg += QStringLiteral("\n\n") + tr("Falhas:") + QStringLiteral("\n")
             + failures.join('\n');
    }
    QMessageBox::information(this, tr("Replace All"), msg);
}
