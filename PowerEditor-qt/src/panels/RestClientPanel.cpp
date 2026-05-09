#include "RestClientPanel.h"

#include <QComboBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

namespace {
QStringList kMethods = {
    QStringLiteral("GET"), QStringLiteral("POST"), QStringLiteral("PUT"),
    QStringLiteral("DELETE"), QStringLiteral("PATCH"),
    QStringLiteral("HEAD"), QStringLiteral("OPTIONS"),
};

// Pretty-print JSON when the server says so (or when the body parses as JSON).
QString maybePrettyJson(const QByteArray& body, const QString& contentType)
{
    const bool looksJson = contentType.contains(QStringLiteral("json"), Qt::CaseInsensitive);
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (looksJson || err.error == QJsonParseError::NoError) {
        if (err.error == QJsonParseError::NoError)
            return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
    }
    return QString::fromUtf8(body);
}
}

RestClientPanel::RestClientPanel(QWidget* parent) : QDockWidget(tr("REST"), parent)
{
    setObjectName(QStringLiteral("RestClientPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);

    m_method = new QComboBox(root);
    m_method->addItems(kMethods);
    m_url    = new QLineEdit(root);
    m_url->setPlaceholderText(tr("https://api.exemplo.com/v1/recurso"));
    m_sendBtn = new QPushButton(tr("Enviar"), root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(m_method);
    row1->addWidget(m_url, 1);
    row1->addWidget(m_sendBtn);

    m_headers = new QTableWidget(0, 2, root);
    m_headers->setHorizontalHeaderLabels({ tr("Header"), tr("Valor") });
    m_headers->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_headers->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_headers->verticalHeader()->setVisible(false);
    m_addHdrBtn = new QPushButton(tr("+ header"), root);
    m_delHdrBtn = new QPushButton(tr("− header"), root);

    auto* row2 = new QHBoxLayout();
    row2->addWidget(new QLabel(tr("Headers:"), root));
    row2->addStretch(1);
    row2->addWidget(m_addHdrBtn);
    row2->addWidget(m_delHdrBtn);

    m_body = new QPlainTextEdit(root);
    m_body->setPlaceholderText(tr("Body (JSON / form / raw)"));
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(10);
    m_body->setFont(mono);

    m_response = new QPlainTextEdit(root);
    m_response->setReadOnly(true);
    m_response->setFont(mono);

    m_status = new QLabel(root);

    auto* split = new QSplitter(Qt::Vertical, root);
    auto* topWrap = new QWidget(root);
    auto* topLay  = new QVBoxLayout(topWrap);
    topLay->setContentsMargins(0, 0, 0, 0);
    topLay->addLayout(row2);
    topLay->addWidget(m_headers);
    topLay->addWidget(new QLabel(tr("Body:"), root));
    topLay->addWidget(m_body, 1);
    auto* bottomWrap = new QWidget(root);
    auto* bottomLay  = new QVBoxLayout(bottomWrap);
    bottomLay->setContentsMargins(0, 0, 0, 0);
    bottomLay->addWidget(new QLabel(tr("Resposta:"), root));
    bottomLay->addWidget(m_response, 1);
    split->addWidget(topWrap);
    split->addWidget(bottomWrap);
    split->setStretchFactor(0, 2);
    split->setStretchFactor(1, 3);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row1);
    lay->addWidget(split, 1);
    lay->addWidget(m_status);
    setWidget(root);

    m_nam = new QNetworkAccessManager(this);

    connect(m_sendBtn,   &QPushButton::clicked, this, &RestClientPanel::onSend);
    connect(m_addHdrBtn, &QPushButton::clicked, this, &RestClientPanel::onAddHeader);
    connect(m_delHdrBtn, &QPushButton::clicked, this, &RestClientPanel::onRemoveHeader);

    loadLast();
}

void RestClientPanel::onAddHeader()
{
    const int row = m_headers->rowCount();
    m_headers->insertRow(row);
    m_headers->setItem(row, 0, new QTableWidgetItem(QString()));
    m_headers->setItem(row, 1, new QTableWidgetItem(QString()));
}
void RestClientPanel::onRemoveHeader()
{
    const int row = m_headers->currentRow();
    if (row >= 0) m_headers->removeRow(row);
}

void RestClientPanel::onSend()
{
    if (m_url->text().trimmed().isEmpty()) {
        m_status->setText(tr("URL vazia."));
        return;
    }
    if (m_reply) { m_reply->abort(); m_reply->deleteLater(); m_reply = nullptr; }

    QNetworkRequest req(QUrl(m_url->text().trimmed()));
    for (int r = 0; r < m_headers->rowCount(); ++r) {
        auto* k = m_headers->item(r, 0);
        auto* v = m_headers->item(r, 1);
        if (!k || k->text().isEmpty()) continue;
        req.setRawHeader(k->text().toUtf8(), v ? v->text().toUtf8() : QByteArray());
    }
    if (req.header(QNetworkRequest::ContentTypeHeader).isNull()
        && (m_method->currentText() == QStringLiteral("POST")
         || m_method->currentText() == QStringLiteral("PUT")
         || m_method->currentText() == QStringLiteral("PATCH"))) {
        // Default to JSON for body-bearing methods unless the user set otherwise.
        req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    }

    const QByteArray body = m_body->toPlainText().toUtf8();
    const QString    m    = m_method->currentText();
    if      (m == QStringLiteral("GET"))     m_reply = m_nam->get(req);
    else if (m == QStringLiteral("POST"))    m_reply = m_nam->post(req, body);
    else if (m == QStringLiteral("PUT"))     m_reply = m_nam->put(req, body);
    else if (m == QStringLiteral("DELETE"))  m_reply = m_nam->deleteResource(req);
    else if (m == QStringLiteral("HEAD"))    m_reply = m_nam->head(req);
    else                                     m_reply = m_nam->sendCustomRequest(req, m.toUtf8(), body);
    if (!m_reply) { m_status->setText(tr("Falha ao montar request.")); return; }

    m_status->setText(tr("Enviando…"));
    persistLast();
    connect(m_reply, &QNetworkReply::finished, this, &RestClientPanel::onReplyFinished);
}

void RestClientPanel::onReplyFinished()
{
    if (!m_reply) return;
    const int code = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString reason = m_reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();

    QStringList lines;
    lines << QStringLiteral("HTTP %1 %2").arg(code).arg(reason);
    for (const auto& pair : m_reply->rawHeaderPairs()) {
        lines << QString::fromUtf8(pair.first) + QStringLiteral(": ")
                + QString::fromUtf8(pair.second);
    }
    lines << QString();
    const QByteArray body = m_reply->readAll();
    const QString contentType = QString::fromUtf8(m_reply->rawHeader("Content-Type"));
    lines << maybePrettyJson(body, contentType);

    m_response->setPlainText(lines.join('\n'));
    m_status->setText(tr("Concluído (%1 bytes)").arg(body.size()));

    m_reply->deleteLater();
    m_reply = nullptr;
}

void RestClientPanel::persistLast() const
{
    QSettings s;
    s.beginGroup(QStringLiteral("RestClient/last"));
    s.setValue(QStringLiteral("method"), m_method->currentText());
    s.setValue(QStringLiteral("url"),    m_url->text());
    QStringList rows;
    for (int r = 0; r < m_headers->rowCount(); ++r) {
        const auto* k = m_headers->item(r, 0);
        const auto* v = m_headers->item(r, 1);
        rows << (k ? k->text() : QString()) + QStringLiteral("\x1F") + (v ? v->text() : QString());
    }
    s.setValue(QStringLiteral("headers"), rows);
    s.setValue(QStringLiteral("body"), m_body->toPlainText());
    s.endGroup();
}

void RestClientPanel::loadLast()
{
    QSettings s;
    s.beginGroup(QStringLiteral("RestClient/last"));
    if (s.contains(QStringLiteral("method")))
        m_method->setCurrentText(s.value(QStringLiteral("method")).toString());
    m_url->setText(s.value(QStringLiteral("url")).toString());
    const QStringList rows = s.value(QStringLiteral("headers")).toStringList();
    for (const QString& r : rows) {
        const QStringList parts = r.split(QChar(0x1F));
        if (parts.size() < 2) continue;
        const int row = m_headers->rowCount();
        m_headers->insertRow(row);
        m_headers->setItem(row, 0, new QTableWidgetItem(parts[0]));
        m_headers->setItem(row, 1, new QTableWidgetItem(parts[1]));
    }
    m_body->setPlainText(s.value(QStringLiteral("body")).toString());
    s.endGroup();
}
