#include "TestRunnerPanel.h"

#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

namespace {
QString frameworkKeyToString(int key) {
    switch (key) {
        case 1: return QStringLiteral("pytest");
        case 2: return QStringLiteral("jest");
        case 3: return QStringLiteral("cargo");
        default: return QString();
    }
}
}

TestRunnerPanel::TestRunnerPanel(QWidget* parent)
    : QDockWidget(tr("Testes"), parent)
{
    setObjectName(QStringLiteral("TestRunnerPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);

    m_workspaceLabel = new QLabel(tr("(sem workspace)"), root);
    m_workspaceLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_frameworkCombo = new QComboBox(root);
    m_frameworkCombo->addItem(tr("Auto"),   0);
    m_frameworkCombo->addItem(QStringLiteral("pytest"), 1);
    m_frameworkCombo->addItem(QStringLiteral("jest"),   2);
    m_frameworkCombo->addItem(QStringLiteral("cargo"),  3);

    m_extraArgs = new QLineEdit(root);
    m_extraArgs->setPlaceholderText(tr("argumentos extra (ex.: -k <expr> ou -- --nocapture)"));

    m_detectBtn = new QPushButton(tr("Detectar"), root);
    m_runBtn    = new QPushButton(tr("Rodar"), root);
    m_stopBtn   = new QPushButton(tr("Parar"), root);
    m_stopBtn->setEnabled(false);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("Framework:"), root));
    row1->addWidget(m_frameworkCombo);
    row1->addWidget(m_detectBtn);
    row1->addStretch(1);
    row1->addWidget(m_runBtn);
    row1->addWidget(m_stopBtn);

    m_tree = new QTreeWidget(root);
    m_tree->setHeaderLabels({ tr("Nome"), tr("Estado"), tr("Detalhes") });
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::Interactive);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(10);
    m_tree->setFont(mono);

    m_status = new QLabel(root);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addWidget(m_workspaceLabel);
    lay->addLayout(row1);
    lay->addWidget(m_extraArgs);
    lay->addWidget(m_tree, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_detectBtn, &QPushButton::clicked, this, &TestRunnerPanel::onDetect);
    connect(m_runBtn,    &QPushButton::clicked, this, &TestRunnerPanel::onRun);
    connect(m_stopBtn,   &QPushButton::clicked, this, &TestRunnerPanel::onStop);
    connect(m_tree,      &QTreeWidget::itemActivated, this, &TestRunnerPanel::onItemActivated);
}

TestRunnerPanel::~TestRunnerPanel()
{
    if (m_proc) {
        m_proc->disconnect(this);
        if (m_proc->state() != QProcess::NotRunning) {
            m_proc->kill();
            m_proc->waitForFinished(500);
        }
        m_proc->deleteLater();
        m_proc = nullptr;
    }
}

void TestRunnerPanel::setWorkspaceFolder(const QString& path)
{
    m_workspaceFolder = path;
    m_workspaceLabel->setText(path.isEmpty() ? tr("(sem workspace)") : path);
    onDetect();
}

TestRunnerPanel::Framework TestRunnerPanel::detectFramework() const
{
    if (m_workspaceFolder.isEmpty()) return Framework::None;
    QDir d(m_workspaceFolder);
    if (d.exists(QStringLiteral("pytest.ini"))     ||
        d.exists(QStringLiteral("conftest.py"))    ||
        d.exists(QStringLiteral("pyproject.toml")))    return Framework::Pytest;
    if (d.exists(QStringLiteral("Cargo.toml")))        return Framework::Cargo;
    if (d.exists(QStringLiteral("package.json"))) {
        QFile f(d.absoluteFilePath(QStringLiteral("package.json")));
        if (f.open(QIODevice::ReadOnly)) {
            const QByteArray content = f.readAll();
            if (content.contains("\"jest\"")) return Framework::Jest;
        }
    }
    return Framework::None;
}

QString TestRunnerPanel::frameworkLabel(Framework f) const
{
    switch (f) {
        case Framework::Pytest: return QStringLiteral("pytest");
        case Framework::Jest:   return QStringLiteral("jest");
        case Framework::Cargo:  return QStringLiteral("cargo");
        default:                return tr("(nenhum)");
    }
}

void TestRunnerPanel::onDetect()
{
    m_framework = detectFramework();
    if (m_framework != Framework::None) {
        const int key = (m_framework == Framework::Pytest) ? 1
                      : (m_framework == Framework::Jest)   ? 2
                      : 3;
        for (int i = 0; i < m_frameworkCombo->count(); ++i) {
            if (m_frameworkCombo->itemData(i).toInt() == key) {
                m_frameworkCombo->setCurrentIndex(i);
                break;
            }
        }
    }
    m_status->setText(tr("Framework detectado: %1").arg(frameworkLabel(m_framework)));
}

void TestRunnerPanel::onRun()
{
    if (m_proc && m_proc->state() != QProcess::NotRunning) return;
    if (m_workspaceFolder.isEmpty()) {
        m_status->setText(tr("Sem workspace ativo."));
        return;
    }

    Framework f = m_framework;
    int key = m_frameworkCombo->currentData().toInt();
    if (key != 0) {
        f = (key == 1) ? Framework::Pytest
          : (key == 2) ? Framework::Jest
          : Framework::Cargo;
    } else if (f == Framework::None) {
        f = detectFramework();
    }
    if (f == Framework::None) {
        m_status->setText(tr("Não consegui detectar framework de teste."));
        return;
    }
    m_framework = f;

    QString cmd;
    QStringList args;
    switch (f) {
        case Framework::Pytest:
            cmd = QStringLiteral("pytest");
            args << QStringLiteral("-v") << QStringLiteral("--no-header")
                 << QStringLiteral("--tb=short");
            break;
        case Framework::Jest:
            cmd = QStringLiteral("npx");
            args << QStringLiteral("jest") << QStringLiteral("--colors=false")
                 << QStringLiteral("--verbose");
            break;
        case Framework::Cargo:
            cmd = QStringLiteral("cargo");
            args << QStringLiteral("test")
                 << QStringLiteral("--color") << QStringLiteral("never");
            break;
        default: return;
    }
    const QString extra = m_extraArgs->text().trimmed();
    if (!extra.isEmpty()) {
        args << QProcess::splitCommand(extra);
    }

    clearTree();
    m_status->setText(tr("Executando %1 %2…").arg(cmd, args.join(' ')));
    m_passed = m_failed = m_skipped = 0;

    if (m_proc) m_proc->deleteLater();
    m_proc = new QProcess(this);
    m_proc->setWorkingDirectory(m_workspaceFolder);
    m_proc->setProcessChannelMode(QProcess::SeparateChannels);
    connect(m_proc, &QProcess::readyReadStandardOutput, this, &TestRunnerPanel::onStdout);
    connect(m_proc, &QProcess::readyReadStandardError,  this, &TestRunnerPanel::onStderr);
    connect(m_proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TestRunnerPanel::onProcessFinished);

    m_proc->start(cmd, args);
    if (!m_proc->waitForStarted(2000)) {
        m_status->setText(tr("Não consegui iniciar %1.").arg(cmd));
        m_proc->deleteLater();
        m_proc = nullptr;
        return;
    }
    m_runBtn->setEnabled(false);
    m_stopBtn->setEnabled(true);
}

void TestRunnerPanel::onStop()
{
    if (m_proc && m_proc->state() != QProcess::NotRunning) m_proc->kill();
}

void TestRunnerPanel::onStdout()
{
    if (!m_proc) return;
    m_stdoutTail += QString::fromUtf8(m_proc->readAllStandardOutput());
    int nl;
    while ((nl = m_stdoutTail.indexOf('\n')) >= 0) {
        QString line = m_stdoutTail.left(nl);
        if (line.endsWith('\r')) line.chop(1);
        m_stdoutTail.remove(0, nl + 1);
        parseLine(line);
    }
}

void TestRunnerPanel::onStderr()
{
    if (!m_proc) return;
    // Cargo emits status to stderr — feed it through the same parser.
    QString s = QString::fromUtf8(m_proc->readAllStandardError());
    for (QString line : s.split('\n')) {
        if (line.endsWith('\r')) line.chop(1);
        if (!line.isEmpty()) parseLine(line);
    }
}

void TestRunnerPanel::onProcessFinished(int exitCode, int /*exitStatus*/)
{
    // Drain trailing partial line.
    if (!m_stdoutTail.isEmpty()) {
        parseLine(m_stdoutTail);
        m_stdoutTail.clear();
    }
    m_status->setText(tr("Concluído (exit=%1)  ✓ %2  ✗ %3  ⊘ %4")
                          .arg(exitCode).arg(m_passed).arg(m_failed).arg(m_skipped));
    m_runBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
}

void TestRunnerPanel::clearTree()
{
    m_tree->clear();
    m_stdoutTail.clear();
}

QTreeWidgetItem* TestRunnerPanel::suiteNode(const QString& name)
{
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto* it = m_tree->topLevelItem(i);
        if (it->text(0) == name) return it;
    }
    auto* it = new QTreeWidgetItem(m_tree);
    it->setText(0, name);
    it->setExpanded(true);
    return it;
}

void TestRunnerPanel::parseLine(const QString& line)
{
    switch (m_framework) {
        case Framework::Pytest: parsePytestLine(line); break;
        case Framework::Jest:   parseJestLine(line);   break;
        case Framework::Cargo:  parseCargoLine(line);  break;
        default: break;
    }
}

void TestRunnerPanel::parsePytestLine(const QString& line)
{
    // pytest -v emits one line per test:
    //   tests/test_foo.py::test_bar PASSED          [ 50%]
    //   tests/test_foo.py::test_baz FAILED          [100%]
    static const QRegularExpression rx(
        QStringLiteral(R"RX(^(\S+\.py)::(\S+)\s+(PASSED|FAILED|ERROR|SKIPPED)\b)RX"));
    auto m = rx.match(line);
    if (!m.hasMatch()) return;
    const QString file   = m.captured(1);
    const QString name   = m.captured(2);
    const QString status = m.captured(3);

    auto* parent = suiteNode(file);
    auto* row = new QTreeWidgetItem(parent);
    row->setText(0, name);
    row->setText(1, status);
    row->setData(0, Qt::UserRole, file);
    if (status == QStringLiteral("PASSED"))      ++m_passed;
    else if (status == QStringLiteral("SKIPPED"))++m_skipped;
    else                                         ++m_failed;
}

void TestRunnerPanel::parseJestLine(const QString& line)
{
    // Jest --verbose --colors=false emits:
    //   PASS  src/foo.test.js
    //     ✓ does the thing (12ms)
    //     ✗ fails this case
    //   FAIL  src/bar.test.js
    static const QRegularExpression rxFile(
        QStringLiteral(R"RX(^(PASS|FAIL)\s+(\S+))RX"));
    static const QRegularExpression rxCase(
        QStringLiteral(R"RX(^\s+([✓✗○])\s+(.+?)(?:\s+\(\d+ms\))?$)RX"));

    if (auto m = rxFile.match(line); m.hasMatch()) {
        suiteNode(m.captured(2));
        return;
    }
    if (auto m = rxCase.match(line); m.hasMatch()) {
        const QString glyph = m.captured(1);
        const QString name  = m.captured(2);
        const QString status = (glyph == QStringLiteral("✓")) ? QStringLiteral("PASSED")
                              : (glyph == QStringLiteral("○")) ? QStringLiteral("SKIPPED")
                              : QStringLiteral("FAILED");
        // Attach to last suite node we saw (the most-recently-added top-level).
        if (m_tree->topLevelItemCount() == 0) suiteNode(QStringLiteral("(sem suite)"));
        auto* parent = m_tree->topLevelItem(m_tree->topLevelItemCount() - 1);
        auto* row = new QTreeWidgetItem(parent);
        row->setText(0, name);
        row->setText(1, status);
        if (status == QStringLiteral("PASSED"))      ++m_passed;
        else if (status == QStringLiteral("SKIPPED"))++m_skipped;
        else                                         ++m_failed;
    }
}

void TestRunnerPanel::parseCargoLine(const QString& line)
{
    // cargo test (--color never) emits:
    //   running 4 tests
    //   test foo::bar::baz ... ok
    //   test foo::quux     ... FAILED
    //   test result: FAILED. 3 passed; 1 failed; 0 ignored; ...
    static const QRegularExpression rxCase(
        QStringLiteral(R"RX(^test\s+(\S+)\s+\.\.\.\s+(ok|FAILED|ignored)\b)RX"));
    auto m = rxCase.match(line);
    if (!m.hasMatch()) return;

    const QString full   = m.captured(1);
    const QString status = m.captured(2);
    const int sep = full.indexOf(QStringLiteral("::"));
    const QString suite = (sep > 0) ? full.left(sep) : QStringLiteral("(root)");
    const QString name  = (sep > 0) ? full.mid(sep + 2) : full;

    auto* parent = suiteNode(suite);
    auto* row = new QTreeWidgetItem(parent);
    row->setText(0, name);
    QString display = (status == QStringLiteral("ok"))      ? QStringLiteral("PASSED")
                    : (status == QStringLiteral("ignored")) ? QStringLiteral("SKIPPED")
                    : QStringLiteral("FAILED");
    row->setText(1, display);
    if (display == QStringLiteral("PASSED"))      ++m_passed;
    else if (display == QStringLiteral("SKIPPED"))++m_skipped;
    else                                          ++m_failed;
}

void TestRunnerPanel::onItemActivated(QTreeWidgetItem* it, int)
{
    if (!it) return;
    const QString filePath = it->data(0, Qt::UserRole).toString();
    if (filePath.isEmpty()) return;
    QString abs = filePath;
    if (!QFileInfo(abs).isAbsolute() && !m_workspaceFolder.isEmpty()) {
        abs = QDir(m_workspaceFolder).absoluteFilePath(filePath);
    }
    emit openFileRequested(abs, 1);
}
