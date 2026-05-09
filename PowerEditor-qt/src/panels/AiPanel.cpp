#include "AiPanel.h"

#include <QAction>
#include <QComboBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

namespace {
constexpr const char* kAnthropic = "anthropic";
constexpr const char* kOpenAi    = "openai";

QString defaultModelFor(const QString& provider) {
    if (provider == QLatin1String(kAnthropic)) return QStringLiteral("claude-haiku-4-5");
    return QStringLiteral("gpt-4o-mini");
}
}

AiPanel::AiPanel(QWidget* parent) : QDockWidget(tr("AI"), parent)
{
    setObjectName(QStringLiteral("AiPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);

    m_provider = new QComboBox(root);
    m_provider->addItem(tr("Anthropic (Claude)"), QString::fromLatin1(kAnthropic));
    m_provider->addItem(tr("OpenAI"),             QString::fromLatin1(kOpenAi));
    m_model = new QComboBox(root);
    m_model->setEditable(true);
    m_keyBtn   = new QPushButton(tr("API key…"), root);
    m_useSelBtn = new QPushButton(tr("Usar seleção"), root);
    m_useSelBtn->setCheckable(true);

    auto* topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel(tr("Provedor:"), root));
    topRow->addWidget(m_provider);
    topRow->addWidget(new QLabel(tr("Modelo:"), root));
    topRow->addWidget(m_model, 1);
    topRow->addWidget(m_keyBtn);
    topRow->addWidget(m_useSelBtn);

    m_transcript = new QPlainTextEdit(root);
    m_transcript->setReadOnly(true);
    m_transcript->setMaximumBlockCount(5000);

    m_prompt = new QPlainTextEdit(root);
    m_prompt->setPlaceholderText(tr("Pergunte algo. Ctrl+Enter envia."));
    m_prompt->setFixedHeight(120);

    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(10);
    m_transcript->setFont(mono);
    m_prompt->setFont(mono);

    m_askBtn = new QPushButton(tr("Perguntar (Ctrl+Enter)"), root);
    m_status = new QLabel(root);

    auto* split = new QSplitter(Qt::Vertical, root);
    split->addWidget(m_transcript);
    split->addWidget(m_prompt);
    split->setStretchFactor(0, 4);
    split->setStretchFactor(1, 1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(topRow);
    lay->addWidget(split, 1);
    lay->addWidget(m_askBtn);
    lay->addWidget(m_status);
    setWidget(root);

    m_nam = new QNetworkAccessManager(this);

    connect(m_provider, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AiPanel::onProviderChanged);
    connect(m_keyBtn, &QPushButton::clicked, this, &AiPanel::onSetApiKey);
    connect(m_askBtn, &QPushButton::clicked, this, &AiPanel::onAsk);

    auto* shortcut = new QAction(this);
    shortcut->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
    connect(shortcut, &QAction::triggered, this, &AiPanel::onAsk);
    m_prompt->addAction(shortcut);

    onProviderChanged(0);
}

void AiPanel::setSelectionContext(const QString& text)
{
    m_selectionContext = text;
}

QString AiPanel::providerKey() const { return m_provider->currentData().toString(); }

QString AiPanel::apiKey() const
{
    QSettings s;
    return s.value(QStringLiteral("Ai/%1/apiKey").arg(providerKey())).toString();
}

void AiPanel::onProviderChanged(int)
{
    const QString p = providerKey();
    m_model->clear();
    if (p == QLatin1String(kAnthropic)) {
        m_model->addItems({ QStringLiteral("claude-haiku-4-5"),
                            QStringLiteral("claude-sonnet-4-6"),
                            QStringLiteral("claude-opus-4-7") });
    } else {
        m_model->addItems({ QStringLiteral("gpt-4o-mini"),
                            QStringLiteral("gpt-4o"),
                            QStringLiteral("gpt-4.1") });
    }
    m_model->setCurrentText(defaultModelFor(p));
    if (apiKey().isEmpty())
        m_status->setText(tr("Defina a API key em \"API key…\" antes de enviar."));
    else
        m_status->setText(tr("Provedor: %1").arg(m_provider->currentText()));
}

void AiPanel::onSetApiKey()
{
    bool ok = false;
    const QString k = QInputDialog::getText(this, tr("API key"),
        tr("Cole a API key do provedor:"), QLineEdit::Password, apiKey(), &ok);
    if (!ok) return;
    QSettings s;
    s.setValue(QStringLiteral("Ai/%1/apiKey").arg(providerKey()), k);
    m_status->setText(tr("API key salva."));
}

void AiPanel::onAsk()
{
    const QString text = m_prompt->toPlainText().trimmed();
    if (text.isEmpty()) return;
    if (apiKey().isEmpty()) {
        QMessageBox::information(this, tr("AI"), tr("Defina a API key primeiro."));
        return;
    }

    QString full;
    if (m_useSelBtn->isChecked() && !m_selectionContext.isEmpty()) {
        full = QStringLiteral("Contexto (seleção do editor):\n```\n%1\n```\n\nPergunta: %2")
                   .arg(m_selectionContext, text);
    } else {
        full = text;
    }

    appendToTranscript(QStringLiteral("você"), text);
    m_prompt->clear();
    m_status->setText(tr("Enviando…"));

    if (providerKey() == QLatin1String(kAnthropic)) postAnthropic(full);
    else                                            postOpenAi(full);
}

void AiPanel::postAnthropic(const QString& prompt)
{
    QNetworkRequest req(QUrl(QStringLiteral("https://api.anthropic.com/v1/messages")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("x-api-key",         apiKey().toUtf8());
    req.setRawHeader("anthropic-version", "2023-06-01");

    QJsonObject body;
    body.insert(QStringLiteral("model"),      m_model->currentText());
    body.insert(QStringLiteral("max_tokens"), 1024);
    QJsonArray messages;
    QJsonObject msg;
    msg.insert(QStringLiteral("role"), QStringLiteral("user"));
    msg.insert(QStringLiteral("content"), prompt);
    messages.append(msg);
    body.insert(QStringLiteral("messages"), messages);

    if (m_reply) m_reply->abort();
    m_reply = m_nam->post(req, QJsonDocument(body).toJson());
    connect(m_reply, &QNetworkReply::finished, this, &AiPanel::onReplyFinished);
}

void AiPanel::postOpenAi(const QString& prompt)
{
    QNetworkRequest req(QUrl(QStringLiteral("https://api.openai.com/v1/chat/completions")));
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    req.setRawHeader("Authorization", QByteArray("Bearer ") + apiKey().toUtf8());

    QJsonObject body;
    body.insert(QStringLiteral("model"), m_model->currentText());
    QJsonArray messages;
    QJsonObject msg;
    msg.insert(QStringLiteral("role"),    QStringLiteral("user"));
    msg.insert(QStringLiteral("content"), prompt);
    messages.append(msg);
    body.insert(QStringLiteral("messages"), messages);

    if (m_reply) m_reply->abort();
    m_reply = m_nam->post(req, QJsonDocument(body).toJson());
    connect(m_reply, &QNetworkReply::finished, this, &AiPanel::onReplyFinished);
}

void AiPanel::onReplyFinished()
{
    if (!m_reply) return;
    const QByteArray bytes = m_reply->readAll();
    const QString contentType = QString::fromUtf8(m_reply->rawHeader("Content-Type"));
    const int code = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    m_reply->deleteLater(); m_reply = nullptr;

    if (code < 200 || code >= 300) {
        appendToTranscript(QStringLiteral("erro"),
            tr("HTTP %1\n%2").arg(code).arg(QString::fromUtf8(bytes)));
        m_status->setText(tr("Erro %1").arg(code));
        return;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(bytes);
    const QJsonObject obj = doc.object();
    QString reply;
    if (providerKey() == QLatin1String(kAnthropic)) {
        // { content: [ { type: "text", text: "..." } ] }
        const QJsonArray arr = obj.value(QStringLiteral("content")).toArray();
        for (const QJsonValue& v : arr) {
            if (v.toObject().value(QStringLiteral("type")).toString() == QStringLiteral("text"))
                reply += v.toObject().value(QStringLiteral("text")).toString();
        }
    } else {
        // { choices: [ { message: { content: "..." } } ] }
        const QJsonArray arr = obj.value(QStringLiteral("choices")).toArray();
        if (!arr.isEmpty()) {
            reply = arr.first().toObject()
                       .value(QStringLiteral("message")).toObject()
                       .value(QStringLiteral("content")).toString();
        }
    }
    if (reply.isEmpty()) reply = QString::fromUtf8(bytes);
    appendToTranscript(QStringLiteral("ai"), reply);
    m_status->setText(tr("Concluído."));
}

void AiPanel::appendToTranscript(const QString& role, const QString& text)
{
    QString block;
    if (!m_transcript->toPlainText().isEmpty()) block += QStringLiteral("\n");
    block += QStringLiteral("─── %1 ───\n%2").arg(role.toUpper(), text);
    m_transcript->appendPlainText(block);
}
