#include "SshExecPanel.h"

#include <QComboBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWidget>

SshExecPanel::SshExecPanel(QWidget* parent) : QDockWidget(tr("SSH Exec"), parent)
{
    setObjectName(QStringLiteral("SshExecPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_host = new QComboBox(root);
    m_host->setEditable(true);
    m_host->setInsertPolicy(QComboBox::NoInsert);
    m_host->lineEdit()->setPlaceholderText(QStringLiteral("user@host ou alias do ~/.ssh/config"));

    m_cmd = new QLineEdit(root); m_cmd->setFont(mono);
    m_cmd->setPlaceholderText(QStringLiteral("ex: uname -a; df -h"));
    m_runBtn   = new QPushButton(tr("Run"), root);
    m_saveBtn  = new QPushButton(tr("Lembrar host"), root);
    m_clearBtn = new QPushButton(tr("Limpar"), root);
    m_out = new QPlainTextEdit(root); m_out->setReadOnly(true); m_out->setFont(mono);
    m_status = new QLabel(root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("Host:"), root));
    row1->addWidget(m_host, 1);
    row1->addWidget(m_saveBtn);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(new QLabel(tr("Cmd:"), root));
    row2->addWidget(m_cmd, 1);
    row2->addWidget(m_runBtn);
    row2->addWidget(m_clearBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(m_out, 1);
    lay->addWidget(m_status);
    setWidget(root);

    loadHosts();

    connect(m_runBtn,   &QPushButton::clicked, this, &SshExecPanel::onRun);
    connect(m_saveBtn,  &QPushButton::clicked, this, &SshExecPanel::onSaveHost);
    connect(m_clearBtn, &QPushButton::clicked, this, &SshExecPanel::onClear);
    connect(m_cmd, &QLineEdit::returnPressed, this, &SshExecPanel::onRun);
}

void SshExecPanel::loadHosts()
{
    QSettings s;
    for (const QString& h : s.value(QStringLiteral("SshExec/hosts")).toStringList())
        m_host->addItem(h);
    // Also pull SFTP recent hosts if present.
    for (const QString& h : s.value(QStringLiteral("Sftp/recent")).toStringList())
        if (m_host->findText(h) < 0) m_host->addItem(h);
}

void SshExecPanel::onSaveHost()
{
    const QString h = m_host->currentText().trimmed();
    if (h.isEmpty()) return;
    QSettings s;
    QStringList hosts = s.value(QStringLiteral("SshExec/hosts")).toStringList();
    if (!hosts.contains(h)) { hosts.prepend(h); s.setValue(QStringLiteral("SshExec/hosts"), hosts); }
    if (m_host->findText(h) < 0) m_host->addItem(h);
    m_status->setText(tr("Host salvo."));
}

void SshExecPanel::onRun()
{
    const QString ssh = QStandardPaths::findExecutable(QStringLiteral("ssh"));
    if (ssh.isEmpty()) { m_status->setText(tr("ssh não encontrado.")); return; }
    const QString h = m_host->currentText().trimmed();
    if (h.isEmpty() || m_cmd->text().trimmed().isEmpty()) {
        m_status->setText(tr("Host e cmd obrigatórios.")); return;
    }
    if (m_proc) { m_proc->kill(); m_proc->deleteLater(); }
    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_proc.data(), &QProcess::readyReadStandardOutput, this, [this]() {
        m_out->appendPlainText(QString::fromUtf8(m_proc->readAllStandardOutput()).trimmed());
    });
    connect(m_proc.data(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus) {
        m_status->setText(tr("Saiu com %1").arg(code));
    });

    m_out->appendPlainText(QStringLiteral("$ %1").arg(m_cmd->text()));
    m_proc->start(ssh, { QStringLiteral("-o"), QStringLiteral("BatchMode=yes"),
                           QStringLiteral("-o"), QStringLiteral("ConnectTimeout=8"),
                           h, m_cmd->text() });
    m_status->setText(tr("Executando…"));
}

void SshExecPanel::onClear() { m_out->clear(); }
