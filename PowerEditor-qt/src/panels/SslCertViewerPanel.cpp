#include "SslCertViewerPanel.h"

#include <QFileDialog>
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

SslCertViewerPanel::SslCertViewerPanel(QWidget* parent) : QDockWidget(tr("SSL Cert"), parent)
{
    setObjectName(QStringLiteral("SslCertViewerPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_pathEdit = new QLineEdit(root); m_pathEdit->setPlaceholderText(tr("arquivo .pem/.crt"));
    m_hostEdit = new QLineEdit(root); m_hostEdit->setPlaceholderText(QStringLiteral("host:port (ex: example.com:443)"));
    m_pickBtn = new QPushButton(tr("…"), root);
    m_loadBtn = new QPushButton(tr("Carregar arquivo"), root);
    m_hostBtn = new QPushButton(tr("Buscar do host"), root);
    m_out = new QPlainTextEdit(root); m_out->setReadOnly(true); m_out->setFont(mono);
    m_status = new QLabel(root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("Arquivo:"), root));
    row1->addWidget(m_pathEdit, 1);
    row1->addWidget(m_pickBtn);
    row1->addWidget(m_loadBtn);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(new QLabel(tr("Host:"), root));
    row2->addWidget(m_hostEdit, 1);
    row2->addWidget(m_hostBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(m_out, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_pickBtn, &QPushButton::clicked, this, &SslCertViewerPanel::onPickFile);
    connect(m_loadBtn, &QPushButton::clicked, this, &SslCertViewerPanel::onLoad);
    connect(m_hostBtn, &QPushButton::clicked, this, &SslCertViewerPanel::onLoadHost);
}

void SslCertViewerPanel::onPickFile()
{
    const QString p = QFileDialog::getOpenFileName(this, tr("Cert"), m_pathEdit->text(),
                                                    tr("Cert (*.pem *.crt *.cer);;Todos (*)"));
    if (!p.isEmpty()) m_pathEdit->setText(p);
}

void SslCertViewerPanel::onLoad()
{
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("openssl"));
    if (tool.isEmpty()) { m_status->setText(tr("openssl não encontrado.")); return; }
    QProcess p;
    p.setProcessChannelMode(QProcess::SeparateChannels);
    p.start(tool, { QStringLiteral("x509"), QStringLiteral("-in"), m_pathEdit->text(),
                     QStringLiteral("-text"), QStringLiteral("-noout") });
    if (!p.waitForFinished(8000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }
    if (p.exitCode() != 0) {
        m_out->setPlainText(QString::fromUtf8(p.readAllStandardError()));
        m_status->setText(tr("Falhou.")); return;
    }
    m_out->setPlainText(QString::fromUtf8(p.readAllStandardOutput()));
    m_status->setText(tr("OK"));
}

void SslCertViewerPanel::onLoadHost()
{
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("openssl"));
    if (tool.isEmpty()) return;
    const QString host = m_hostEdit->text().trimmed();
    if (host.isEmpty()) return;
    QProcess sclient;
    sclient.setProcessChannelMode(QProcess::SeparateChannels);
    sclient.start(tool, { QStringLiteral("s_client"),
                            QStringLiteral("-connect"), host,
                            QStringLiteral("-servername"), host.section(':', 0, 0),
                            QStringLiteral("-showcerts") });
    sclient.write("\n"); // close stdin
    sclient.closeWriteChannel();
    if (!sclient.waitForFinished(10000)) { sclient.kill(); m_status->setText(tr("Timeout.")); return; }
    QByteArray pem;
    bool inCert = false;
    for (const QByteArray& line : sclient.readAllStandardOutput().split('\n')) {
        if (line.contains("BEGIN CERTIFICATE"))      inCert = true;
        if (inCert) { pem += line; pem += '\n'; }
        if (line.contains("END CERTIFICATE"))        { inCert = false; break; }
    }
    if (pem.isEmpty()) { m_status->setText(tr("Sem cert no stream.")); return; }

    QProcess x509;
    x509.setProcessChannelMode(QProcess::SeparateChannels);
    x509.start(tool, { QStringLiteral("x509"), QStringLiteral("-text"), QStringLiteral("-noout") });
    x509.write(pem); x509.closeWriteChannel();
    if (!x509.waitForFinished(5000)) { x509.kill(); return; }
    m_out->setPlainText(QString::fromUtf8(x509.readAllStandardOutput()));
    m_status->setText(tr("OK — %1").arg(host));
}
