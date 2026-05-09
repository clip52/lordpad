#include "BuildWatchPanel.h"

#include <QCheckBox>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileSystemWatcher>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

BuildWatchPanel::BuildWatchPanel(QWidget* parent) : QDockWidget(tr("Build Watch"), parent)
{
    setObjectName(QStringLiteral("BuildWatchPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_dirEdit = new QLineEdit(root);
    m_dirEdit->setPlaceholderText(QStringLiteral("Diretório a observar"));
    m_cmdEdit = new QLineEdit(root);
    m_cmdEdit->setPlaceholderText(QStringLiteral("ex: make -j4  |  cmake --build build  |  npm test"));
    m_pickBtn   = new QPushButton(tr("…"), root);
    m_runBtn    = new QPushButton(tr("Rodar agora"), root);
    m_watchBtn  = new QPushButton(tr("Iniciar watch"), root); m_watchBtn->setCheckable(true);
    m_recursiveBox = new QCheckBox(tr("recursivo"), root); m_recursiveBox->setChecked(true);
    m_log = new QPlainTextEdit(root); m_log->setReadOnly(true); m_log->setFont(mono);
    m_status = new QLabel(root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("Dir:"), root));
    row1->addWidget(m_dirEdit, 1);
    row1->addWidget(m_pickBtn);
    row1->addWidget(m_recursiveBox);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(new QLabel(tr("Cmd:"), root));
    row2->addWidget(m_cmdEdit, 1);
    row2->addWidget(m_runBtn);
    row2->addWidget(m_watchBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(m_log, 1);
    lay->addWidget(m_status);
    setWidget(root);

    m_watcher = new QFileSystemWatcher(this);
    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true); m_debounce->setInterval(400);
    connect(m_debounce, &QTimer::timeout, this, &BuildWatchPanel::onTrigger);
    connect(m_watcher, &QFileSystemWatcher::fileChanged,      this, &BuildWatchPanel::onChanged);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &BuildWatchPanel::onChanged);

    QSettings s;
    m_dirEdit->setText(s.value(QStringLiteral("BuildWatch/dir")).toString());
    m_cmdEdit->setText(s.value(QStringLiteral("BuildWatch/cmd")).toString());

    connect(m_pickBtn,  &QPushButton::clicked, this, &BuildWatchPanel::onPickDir);
    connect(m_runBtn,   &QPushButton::clicked, this, &BuildWatchPanel::onRunOnce);
    connect(m_watchBtn, &QPushButton::clicked, this, &BuildWatchPanel::onToggleWatch);
    connect(m_dirEdit, &QLineEdit::editingFinished, this, [this]() {
        QSettings().setValue(QStringLiteral("BuildWatch/dir"), m_dirEdit->text());
    });
    connect(m_cmdEdit, &QLineEdit::editingFinished, this, [this]() {
        QSettings().setValue(QStringLiteral("BuildWatch/cmd"), m_cmdEdit->text());
    });
}

BuildWatchPanel::~BuildWatchPanel() = default;

void BuildWatchPanel::onPickDir()
{
    const QString p = QFileDialog::getExistingDirectory(this, tr("Selecione diretório"), m_dirEdit->text());
    if (!p.isEmpty()) m_dirEdit->setText(p);
}

void BuildWatchPanel::onRunOnce() { startProcess(); }

void BuildWatchPanel::onToggleWatch()
{
    const QStringList existing = m_watcher->directories() + m_watcher->files();
    if (!existing.isEmpty()) m_watcher->removePaths(existing);

    if (m_watchBtn->isChecked()) {
        const QString dir = m_dirEdit->text().trimmed();
        if (dir.isEmpty() || !QDir(dir).exists()) {
            m_status->setText(tr("Diretório inválido."));
            m_watchBtn->setChecked(false); return;
        }
        QStringList paths; paths << dir;
        if (m_recursiveBox->isChecked()) {
            QDirIterator it(dir, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QString sub = it.next();
                if (sub.contains(QStringLiteral("/.git/")) || sub.endsWith(QStringLiteral("/.git"))) continue;
                if (sub.contains(QStringLiteral("/node_modules"))) continue;
                if (sub.contains(QStringLiteral("/build")))        continue;
                paths << sub;
                if (paths.size() > 1024) break;
            }
        }
        m_watcher->addPaths(paths);
        m_watching = true;
        m_watchBtn->setText(tr("Parar watch"));
        m_status->setText(tr("Observando %1 caminho(s).").arg(paths.size()));
    } else {
        m_watching = false;
        m_watchBtn->setText(tr("Iniciar watch"));
        m_status->setText(tr("Watch desativado."));
    }
}

void BuildWatchPanel::onChanged(const QString& /*path*/)
{
    if (!m_watching) return;
    m_debounce->start();
}

void BuildWatchPanel::onTrigger()
{
    if (!m_watching) return;
    m_log->appendPlainText(QStringLiteral("\n— %1 — re-rodando build —")
                              .arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
    startProcess();
}

void BuildWatchPanel::startProcess()
{
    if (m_proc) { m_proc->kill(); m_proc->deleteLater(); }
    const QString dir = m_dirEdit->text().trimmed();
    const QString cmd = m_cmdEdit->text().trimmed();
    if (cmd.isEmpty()) { m_status->setText(tr("Sem comando.")); return; }

    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::MergedChannels);
    if (!dir.isEmpty()) m_proc->setWorkingDirectory(dir);
    connect(m_proc.data(), &QProcess::readyReadStandardOutput, this, [this]() {
        m_log->appendPlainText(QString::fromLocal8Bit(m_proc->readAllStandardOutput()).trimmed());
    });
    connect(m_proc.data(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus) {
        m_status->setText(tr("Saiu com %1").arg(code));
    });

    const QString shell = QStandardPaths::findExecutable(QStringLiteral("bash"));
    if (!shell.isEmpty()) {
        m_proc->start(shell, { QStringLiteral("-lc"), cmd });
    } else {
        const QStringList toks = QProcess::splitCommand(cmd);
        if (toks.isEmpty()) return;
        m_proc->start(toks.first(), toks.mid(1));
    }
    m_status->setText(tr("Executando…"));
}
