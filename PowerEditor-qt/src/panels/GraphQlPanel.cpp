#include "GraphQlPanel.h"

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

GraphQlPanel::GraphQlPanel(QWidget* parent) : QDockWidget(tr("GraphQL"), parent)
{
    setObjectName(QStringLiteral("GraphQlPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_url = new QLineEdit(root);
    m_url->setPlaceholderText(QStringLiteral("https://api.exemplo.com/graphql"));
    m_authHeader = new QLineEdit(root);
    m_authHeader->setPlaceholderText(QStringLiteral("Authorization: Bearer ..."));
    m_query = new QPlainTextEdit(root); m_query->setFont(mono);
    m_query->setPlaceholderText(QStringLiteral("query { ... }"));
    m_vars  = new QPlainTextEdit(root); m_vars->setFont(mono);
    m_vars->setPlaceholderText(QStringLiteral("{ \"id\": 1 }   (variables JSON)"));
    m_response = new QPlainTextEdit(root); m_response->setReadOnly(true); m_response->setFont(mono);
    m_sendBtn  = new QPushButton(tr("Enviar"), root);
    m_status   = new QLabel(root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("URL:"), root)); row1->addWidget(m_url, 1); row1->addWidget(m_sendBtn);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(new QLabel(tr("Header:"), root)); row2->addWidget(m_authHeader, 1);

    auto* split = new QSplitter(Qt::Vertical, root);
    auto* topW = new QWidget(); auto* topL = new QVBoxLayout(topW);
    topL->setContentsMargins(0,0,0,0);
    topL->addWidget(new QLabel(tr("Query:"))); topL->addWidget(m_query, 2);
    topL->addWidget(new QLabel(tr("Variables (JSON):"))); topL->addWidget(m_vars, 1);
    split->addWidget(topW); split->addWidget(m_response);
    split->setStretchFactor(0, 2); split->setStretchFactor(1, 3);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(split, 1);
    lay->addWidget(m_status);
    setWidget(root);

    m_nam = new QNetworkAccessManager(this);
    connect(m_sendBtn, &QPushButton::clicked, this, &GraphQlPanel::onSend);

    QSettings s; s.beginGroup(QStringLiteral("GraphQL/last"));
    m_url->setText(s.value(QStringLiteral("url")).toString());
    m_query->setPlainText(s.value(QStringLiteral("query")).toString());
    m_vars->setPlainText(s.value(QStringLiteral("vars")).toString());
    m_authHeader->setText(s.value(QStringLiteral("auth")).toString());
    s.endGroup();
}

void GraphQlPanel::onSend()
{
    if (m_url->text().trimmed().isEmpty()) { m_status->setText(tr("URL vazia.")); return; }
    QSettings s; s.beginGroup(QStringLiteral("GraphQL/last"));
    s.setValue(QStringLiteral("url"),   m_url->text());
    s.setValue(QStringLiteral("query"), m_query->toPlainText());
    s.setValue(QStringLiteral("vars"),  m_vars->toPlainText());
    s.setValue(QStringLiteral("auth"),  m_authHeader->text());
    s.endGroup();

    QNetworkRequest req(QUrl(m_url->text().trimmed()));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!m_authHeader->text().trimmed().isEmpty()) {
        // Aceita formato "Header: Value" ou só "Bearer xxx".
        const QString line = m_authHeader->text().trimmed();
        const int colon = line.indexOf(':');
        if (colon > 0) {
            req.setRawHeader(line.left(colon).toUtf8(), line.mid(colon + 1).trimmed().toUtf8());
        } else {
            req.setRawHeader("Authorization", line.toUtf8());
        }
    }

    QJsonObject body;
    body.insert(QStringLiteral("query"), m_query->toPlainText());
    if (!m_vars->toPlainText().trimmed().isEmpty()) {
        QJsonParseError pe{};
        const QJsonDocument vd = QJsonDocument::fromJson(m_vars->toPlainText().toUtf8(), &pe);
        if (pe.error != QJsonParseError::NoError) {
            m_status->setText(tr("Variables JSON inválido: %1").arg(pe.errorString()));
            return;
        }
        body.insert(QStringLiteral("variables"), vd.object());
    }

    if (m_reply) { m_reply->abort(); m_reply->deleteLater(); m_reply = nullptr; }
    m_status->setText(tr("Enviando…"));
    m_reply = m_nam->post(req, QJsonDocument(body).toJson());
    connect(m_reply, &QNetworkReply::finished, this, &GraphQlPanel::onReplyFinished);
}

void GraphQlPanel::onReplyFinished()
{
    if (!m_reply) return;
    const QByteArray bytes = m_reply->readAll();
    const int code = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    m_reply->deleteLater(); m_reply = nullptr;
    QJsonParseError pe{};
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &pe);
    if (pe.error == QJsonParseError::NoError) {
        m_response->setPlainText(QString::fromUtf8(doc.toJson(QJsonDocument::Indented)));
    } else {
        m_response->setPlainText(QString::fromUtf8(bytes));
    }
    m_status->setText(tr("HTTP %1, %2 bytes").arg(code).arg(bytes.size()));
}
