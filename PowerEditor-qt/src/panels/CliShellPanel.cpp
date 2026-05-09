#include "CliShellPanel.h"

#include <QComboBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

CliShellPanel::CliShellPanel(QWidget* parent) : QDockWidget(tr("CLI DB"), parent)
{
    setObjectName(QStringLiteral("CliShellPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_flavor = new QComboBox(root);
    m_flavor->addItems({ QStringLiteral("mysql"), QStringLiteral("mongosh"),
                         QStringLiteral("redis-cli"), QStringLiteral("psql") });
    m_uri = new QLineEdit(root);
    m_uri->setPlaceholderText(QStringLiteral("ex: mysql://user:pwd@host/db | mongodb://… | redis://… | postgres://…"));
    m_connectBtn = new QPushButton(tr("Conectar"), root);
    m_runBtn     = new QPushButton(tr("Executar"), root);
    m_clearBtn   = new QPushButton(tr("Limpar"), root);

    m_query = new QPlainTextEdit(root); m_query->setFont(mono); m_query->setMaximumHeight(120);
    m_out   = new QPlainTextEdit(root); m_out->setFont(mono); m_out->setReadOnly(true);
    m_status = new QLabel(root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("Sabor:"), root)); row1->addWidget(m_flavor);
    row1->addWidget(m_uri, 1);
    row1->addWidget(m_connectBtn);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(m_runBtn);
    row2->addWidget(m_clearBtn);
    row2->addStretch(1);

    auto* split = new QSplitter(Qt::Vertical, root);
    split->addWidget(m_query); split->addWidget(m_out);
    split->setStretchFactor(0, 1); split->setStretchFactor(1, 3);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(split, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_connectBtn, &QPushButton::clicked, this, &CliShellPanel::onConnect);
    connect(m_runBtn,     &QPushButton::clicked, this, &CliShellPanel::onRun);
    connect(m_clearBtn,   &QPushButton::clicked, this, &CliShellPanel::onClear);
}

QString CliShellPanel::flavorTool() const
{
    const QString f = m_flavor->currentText();
    if (f == QStringLiteral("mysql"))     return QStandardPaths::findExecutable(QStringLiteral("mysql"));
    if (f == QStringLiteral("mongosh"))   return QStandardPaths::findExecutable(QStringLiteral("mongosh"));
    if (f == QStringLiteral("redis-cli")) return QStandardPaths::findExecutable(QStringLiteral("redis-cli"));
    if (f == QStringLiteral("psql"))      return QStandardPaths::findExecutable(QStringLiteral("psql"));
    return {};
}

QStringList CliShellPanel::flavorArgs() const
{
    const QString f   = m_flavor->currentText();
    const QString uri = m_uri->text().trimmed();
    if (f == QStringLiteral("mysql"))     return { QStringLiteral("--connect-expired-password"),
                                                    QStringLiteral("-A"),
                                                    QStringLiteral("--connect-timeout=4"),
                                                    QStringLiteral("--protocol=tcp"),
                                                    QStringLiteral("-e"), m_query->toPlainText(),
                                                    QStringLiteral("-h"), QUrl(uri).host(),
                                                    QStringLiteral("-u"), QUrl(uri).userName() };
    if (f == QStringLiteral("mongosh"))   return { uri, QStringLiteral("--quiet"),
                                                    QStringLiteral("--eval"), m_query->toPlainText() };
    if (f == QStringLiteral("redis-cli")) return { QStringLiteral("-u"), uri };
    if (f == QStringLiteral("psql"))      return { uri, QStringLiteral("-c"), m_query->toPlainText() };
    return {};
}

void CliShellPanel::onConnect()
{
    const QString tool = flavorTool();
    if (tool.isEmpty()) { m_status->setText(tr("Tool não encontrada no PATH.")); return; }
    m_status->setText(tr("OK — %1").arg(tool));
}

void CliShellPanel::onRun()
{
    const QString tool = flavorTool();
    if (tool.isEmpty()) { m_status->setText(tr("Tool não encontrada.")); return; }
    if (m_proc) { m_proc->kill(); m_proc->deleteLater(); }
    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::MergedChannels);

    const QString flavor = m_flavor->currentText();
    QStringList args = flavorArgs();
    QByteArray stdinData;
    if (flavor == QStringLiteral("redis-cli")) stdinData = m_query->toPlainText().toUtf8();

    connect(m_proc.data(), &QProcess::readyReadStandardOutput, this, [this]() {
        m_out->appendPlainText(QString::fromLocal8Bit(m_proc->readAllStandardOutput()));
    });
    connect(m_proc.data(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this](int code, QProcess::ExitStatus) {
        m_status->setText(tr("Saiu com %1").arg(code));
    });

    m_status->setText(tr("Executando…"));
    m_proc->start(tool, args);
    if (!stdinData.isEmpty()) {
        m_proc->write(stdinData);
        m_proc->closeWriteChannel();
    }
}

void CliShellPanel::onClear() { m_out->clear(); }
