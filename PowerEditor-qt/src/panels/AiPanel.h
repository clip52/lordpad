#pragma once

#include <QDockWidget>
#include <QString>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;

// AiPanel (M16) — minimal AI chat for the editor.
//
// Provider combo picks Anthropic (Claude) or OpenAI; the API key lives in
// QSettings under "Ai/<provider>/apiKey". A "Usar seleção do editor"
// checkbox prefixes the active editor's selection to the prompt as context.
//
// The protocol surface is intentionally tiny: we hit /v1/messages on
// Anthropic and /v1/chat/completions on OpenAI with the user's prompt and
// extract the first text block. Streaming, tools, multi-turn history etc.
// are out of scope for this MVP.
class AiPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit AiPanel(QWidget* parent = nullptr);

    // Optional accessors used by MainWindow to feed the active editor's
    // selected text in as prompt context.
    void setSelectionContext(const QString& text);

private slots:
    void onAsk();
    void onSetApiKey();
    void onReplyFinished();
    void onProviderChanged(int);

private:
    void postAnthropic(const QString& prompt);
    void postOpenAi(const QString& prompt);
    void appendToTranscript(const QString& role, const QString& text);
    QString providerKey() const;
    QString apiKey() const;

    QComboBox*      m_provider = nullptr;
    QComboBox*      m_model    = nullptr;
    QPlainTextEdit* m_transcript = nullptr;
    QPlainTextEdit* m_prompt   = nullptr;
    QPushButton*    m_askBtn   = nullptr;
    QPushButton*    m_keyBtn   = nullptr;
    QPushButton*    m_useSelBtn = nullptr;
    QLabel*         m_status   = nullptr;

    QString m_selectionContext;

    QNetworkAccessManager* m_nam = nullptr;
    QNetworkReply*         m_reply = nullptr;
};
