#include "AiGhostCompletion.h"

#include <QEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QTimer>
#include <QUrl>

#include <ScintillaEdit.h>
#include <ScintillaTypes.h>

namespace {
constexpr const char* kEnabledKey = "AiGhost/Enabled";
constexpr int kIdleMs = 1200;
constexpr int kCtxBytes = 800;
}

AiGhostCompletion::AiGhostCompletion(QObject* parent) : QObject(parent)
{
    QSettings s; m_enabled = s.value(kEnabledKey, false).toBool();
    m_idleTimer = new QTimer(this);
    m_idleTimer->setSingleShot(true);
    m_idleTimer->setInterval(kIdleMs);
    connect(m_idleTimer, &QTimer::timeout, this, &AiGhostCompletion::onIdleTimeout);
    m_nam = new QNetworkAccessManager(this);
}

void AiGhostCompletion::setEnabled(bool on)
{
    if (m_enabled == on) return;
    m_enabled = on;
    QSettings s; s.setValue(kEnabledKey, on);
    if (!on) {
        for (auto it = m_ghosts.begin(); it != m_ghosts.end(); ++it) clearGhost(it.key());
        m_ghosts.clear();
    }
}

void AiGhostCompletion::attach(ScintillaEdit* editor)
{
    if (!editor) return;
    editor->installEventFilter(this);
    connect(editor, &ScintillaEditBase::modified, this, [this, editor](
                Scintilla::ModificationFlags type,
                Scintilla::Position, Scintilla::Position,
                Scintilla::Position, const QByteArray&,
                Scintilla::Position, Scintilla::FoldLevel, Scintilla::FoldLevel) {
        if (!m_enabled) return;
        const auto mask = Scintilla::ModificationFlags::InsertText
                        | Scintilla::ModificationFlags::DeleteText;
        if (static_cast<int>(type & mask) == 0) return;
        clearGhost(editor);
        m_ghosts.remove(editor);
        scheduleRequest(editor);
    });
    connect(editor, &QObject::destroyed, this, [this](QObject* obj) {
        m_ghosts.remove(reinterpret_cast<ScintillaEdit*>(obj));
    });
}

void AiGhostCompletion::detach(ScintillaEdit* editor)
{
    if (!editor) return;
    editor->removeEventFilter(this);
    clearGhost(editor);
    m_ghosts.remove(editor);
}

void AiGhostCompletion::scheduleRequest(ScintillaEdit* sci)
{
    if (!sci || !m_enabled) return;
    m_pending = sci;
    m_idleTimer->start();
}

void AiGhostCompletion::onIdleTimeout()
{
    auto* sci = m_pending.data();
    if (!sci || !m_enabled) return;
    const sptr_t pos = sci->currentPos();
    const sptr_t prefStart = qMax<sptr_t>(0, pos - kCtxBytes);
    const sptr_t sufEnd    = qMin<sptr_t>(sci->length(), pos + 200);
    const QString prefix = QString::fromUtf8(sci->textRange(prefStart, pos));
    const QString suffix = QString::fromUtf8(sci->textRange(pos, sufEnd));
    sendRequest(sci, prefix, suffix);
}

void AiGhostCompletion::sendRequest(ScintillaEdit* sci, const QString& prefix, const QString& suffix)
{
    QSettings s;
    const QString provider = s.value(QStringLiteral("Ai/anthropic/apiKey")).toString().isEmpty()
        ? QStringLiteral("openai") : QStringLiteral("anthropic");
    const QString key = s.value(QStringLiteral("Ai/%1/apiKey").arg(provider)).toString();
    if (key.isEmpty()) return;   // silently no-op; status bar can't help here

    if (m_reply) { m_reply->abort(); m_reply->deleteLater(); m_reply = nullptr; }

    const QString promptUser = QStringLiteral(
        "Você é um auto-complete inline. Devolva APENAS o texto a inserir "
        "logo após o caret — sem explicações, sem markdown. Mantenha estilo, "
        "indentação e idioma do contexto. Máx 1-3 linhas.\n\n"
        "PREFIXO:\n%1\n\n[CARET]\n\nSUFIXO:\n%2").arg(prefix, suffix);

    QNetworkRequest req;
    QJsonObject body;
    if (provider == QStringLiteral("anthropic")) {
        req.setUrl(QUrl(QStringLiteral("https://api.anthropic.com/v1/messages")));
        req.setRawHeader("x-api-key",         key.toUtf8());
        req.setRawHeader("anthropic-version", "2023-06-01");
        body.insert("model", QStringLiteral("claude-haiku-4-5"));
        body.insert("max_tokens", 200);
        QJsonArray msgs;
        QJsonObject m; m.insert("role", "user"); m.insert("content", promptUser);
        msgs.append(m);
        body.insert("messages", msgs);
    } else {
        req.setUrl(QUrl(QStringLiteral("https://api.openai.com/v1/chat/completions")));
        req.setRawHeader("Authorization", QByteArray("Bearer ") + key.toUtf8());
        body.insert("model", QStringLiteral("gpt-4o-mini"));
        QJsonArray msgs;
        QJsonObject m; m.insert("role", "user"); m.insert("content", promptUser);
        msgs.append(m);
        body.insert("messages", msgs);
    }
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    Ghost g; g.caretAtFetch = static_cast<int>(sci->currentPos());
    m_ghosts[sci] = g;

    m_reply = m_nam->post(req, QJsonDocument(body).toJson());
    QPointer<ScintillaEdit> sciPtr(sci);
    connect(m_reply, &QNetworkReply::finished, this, [this, sciPtr]() {
        if (!m_reply) return;
        const QByteArray bytes = m_reply->readAll();
        const int code = m_reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        m_reply->deleteLater(); m_reply = nullptr;
        if (code < 200 || code >= 300 || !sciPtr) return;
        const QJsonObject obj = QJsonDocument::fromJson(bytes).object();
        QString text;
        if (obj.contains("content")) {
            for (const QJsonValue& v : obj.value("content").toArray()) {
                if (v.toObject().value("type").toString() == "text")
                    text += v.toObject().value("text").toString();
            }
        } else if (obj.contains("choices")) {
            const auto arr = obj.value("choices").toArray();
            if (!arr.isEmpty()) text = arr.first().toObject()
                                          .value("message").toObject()
                                          .value("content").toString();
        }
        text = text.trimmed();
        if (text.isEmpty()) return;
        // Take only first 3 lines.
        QStringList lines = text.split('\n');
        if (lines.size() > 3) lines = lines.mid(0, 3);
        text = lines.join('\n');

        // Bail if caret moved (user kept typing).
        auto it = m_ghosts.find(sciPtr.data());
        if (it == m_ghosts.end()) return;
        if (it->caretAtFetch != static_cast<int>(sciPtr->currentPos())) return;
        it->text = text;
        renderGhost(sciPtr.data(), text);
    });
}

void AiGhostCompletion::renderGhost(ScintillaEdit* sci, const QString& text)
{
    if (!sci) return;
    const int line = static_cast<int>(sci->lineFromPosition(sci->currentPos()));
    sci->annotationSetVisible(static_cast<int>(Scintilla::AnnotationVisible::Boxed));
    // Style 0 default — themes already render annotations subtly. Prefix the
    // text with "» " so it's distinguishable from real content.
    const QString display = QStringLiteral("» ") + text;
    sci->annotationSetText(line, display.toUtf8().constData());
    sci->annotationSetStyle(line, 0);
}

void AiGhostCompletion::clearGhost(ScintillaEdit* sci)
{
    if (!sci) return;
    sci->annotationClearAll();
}

bool AiGhostCompletion::acceptCurrent(ScintillaEdit* editor)
{
    if (!editor) return false;
    auto it = m_ghosts.find(editor);
    if (it == m_ghosts.end() || it->text.isEmpty()) return false;
    const QByteArray ins = it->text.toUtf8();
    editor->beginUndoAction();
    editor->insertText(editor->currentPos(), ins.constData());
    editor->gotoPos(editor->currentPos() + ins.size());
    editor->endUndoAction();
    clearGhost(editor);
    m_ghosts.erase(it);
    return true;
}

bool AiGhostCompletion::dismissCurrent(ScintillaEdit* editor)
{
    if (!editor) return false;
    auto it = m_ghosts.find(editor);
    if (it == m_ghosts.end()) return false;
    clearGhost(editor);
    m_ghosts.erase(it);
    return true;
}

bool AiGhostCompletion::eventFilter(QObject*, QEvent*)
{
    return false;   // hooks handled via MainWindow's filter for Tab/Esc
}
