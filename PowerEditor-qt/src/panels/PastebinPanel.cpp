#include "PastebinPanel.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QHBoxLayout>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

#include <ScintillaEdit.h>

PastebinPanel::PastebinPanel(QWidget* parent) : QDockWidget(tr("Pastebin"), parent)
{
    setObjectName(QStringLiteral("PastebinPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);
    auto* root = new QWidget(this);
    m_endpoint = new QLineEdit(root);
    m_endpoint->setPlaceholderText(QStringLiteral("https://0x0.st  (POST multipart 'file=...' )"));
    QSettings s;
    m_endpoint->setText(s.value(QStringLiteral("Pastebin/endpoint"), QStringLiteral("https://0x0.st")).toString());
    m_btn = new QPushButton(tr("Upload seleção/buffer"), root);
    m_log = new QPlainTextEdit(root); m_log->setReadOnly(true);
    m_status = new QLabel(root);
    auto* row = new QHBoxLayout();
    row->addWidget(new QLabel(tr("Endpoint:"), root));
    row->addWidget(m_endpoint, 1);
    row->addWidget(m_btn);
    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row);
    lay->addWidget(m_log, 1);
    lay->addWidget(m_status);
    setWidget(root);

    m_nam = new QNetworkAccessManager(this);
    connect(m_btn, &QPushButton::clicked, this, &PastebinPanel::onUpload);
}

void PastebinPanel::onUpload()
{
    if (!m_editor) { m_status->setText(tr("Sem editor.")); return; }
    QSettings s; s.setValue(QStringLiteral("Pastebin/endpoint"), m_endpoint->text());
    QByteArray bytes = m_editor->getSelText();
    if (bytes.isEmpty()) bytes = m_editor->getText(m_editor->textLength() + 1);
    if (bytes.isEmpty()) { m_status->setText(tr("Vazio.")); return; }

    auto* multi = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
        QVariant(QStringLiteral("form-data; name=\"file\"; filename=\"upload.txt\"")));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader,
        QVariant(QStringLiteral("text/plain; charset=utf-8")));
    filePart.setBody(bytes);
    multi->append(filePart);

    QNetworkRequest req(QUrl(m_endpoint->text().trimmed()));
    auto* rep = m_nam->post(req, multi);
    multi->setParent(rep);
    m_status->setText(tr("Enviando…"));
    connect(rep, &QNetworkReply::finished, this, [this, rep]() {
        const QByteArray body = rep->readAll();
        const int code = rep->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        rep->deleteLater();
        if (code < 200 || code >= 300) {
            m_log->appendPlainText(tr("[%1] HTTP %2: %3")
                .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
                .arg(code).arg(QString::fromUtf8(body).trimmed()));
            m_status->setText(tr("Falha %1").arg(code));
            return;
        }
        const QString url = QString::fromUtf8(body).trimmed();
        m_log->appendPlainText(QStringLiteral("[%1] %2")
            .arg(QDateTime::currentDateTime().toString(Qt::ISODate)).arg(url));
        QApplication::clipboard()->setText(url);
        m_status->setText(tr("URL copiada pro clipboard."));
    });
}
