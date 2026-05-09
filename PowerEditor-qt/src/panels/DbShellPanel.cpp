#include "DbShellPanel.h"

#include <QComboBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWidget>

DbShellPanel::DbShellPanel(QWidget* parent) : QDockWidget(tr("DB shell"), parent)
{
    setObjectName(QStringLiteral("DbShellPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_provider = new QComboBox(root);
    m_provider->addItems({ QStringLiteral("psql (Postgres)"), QStringLiteral("redis-cli") });
    m_conn = new QLineEdit(root);
    m_conn->setPlaceholderText(QStringLiteral("Postgres: postgres://user:pass@host:5432/db   |   Redis: -h host -p 6379"));

    m_query = new QPlainTextEdit(root);  m_query->setFont(mono);
    m_query->setPlaceholderText(tr("Comando / query"));
    m_output = new QPlainTextEdit(root); m_output->setReadOnly(true); m_output->setFont(mono);
    m_runBtn = new QPushButton(tr("Run"), root);
    m_status = new QLabel(root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("Provider:"), root));
    row1->addWidget(m_provider);
    row1->addWidget(m_conn, 1);
    row1->addWidget(m_runBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row1);
    lay->addWidget(m_query, 1);
    lay->addWidget(m_output, 2);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_runBtn, &QPushButton::clicked, this, &DbShellPanel::onRun);
}

void DbShellPanel::onRun()
{
    QString exe;
    QStringList args;
    if (m_provider->currentText().startsWith(QStringLiteral("psql"))) {
        exe = QStandardPaths::findExecutable(QStringLiteral("psql"));
        if (exe.isEmpty()) { m_status->setText(tr("psql não encontrado no PATH.")); return; }
        const QString conn = m_conn->text().trimmed();
        if (!conn.isEmpty()) args << conn;
        args << QStringLiteral("-c") << m_query->toPlainText();
    } else {
        exe = QStandardPaths::findExecutable(QStringLiteral("redis-cli"));
        if (exe.isEmpty()) { m_status->setText(tr("redis-cli não encontrado.")); return; }
        // Conn é repassado tal qual (usuário passa "-h host -p 6379" etc.).
        const QStringList connArgs = QProcess::splitCommand(m_conn->text());
        args << connArgs;
        const QStringList cmdArgs = QProcess::splitCommand(m_query->toPlainText());
        args << cmdArgs;
    }

    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(exe, args);
    if (!p.waitForStarted(2000)) { m_status->setText(tr("Falha ao iniciar.")); return; }
    if (!p.waitForFinished(30000)) { p.kill(); p.waitForFinished(500); m_status->setText(tr("Timeout.")); return; }
    m_output->setPlainText(QString::fromUtf8(p.readAllStandardOutput()));
    m_status->setText(tr("exit=%1").arg(p.exitCode()));
}
