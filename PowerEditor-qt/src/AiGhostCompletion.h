#pragma once

#include <QObject>
#include <QPointer>
#include <QString>
#include <QHash>
#include <QTimer>

class ScintillaEdit;
class QNetworkAccessManager;
class QNetworkReply;
class QKeyEvent;

// AiGhostCompletion (M21) — inline AI ghost suggestions.
//
// Idle do usuário (debounce 1.2s) → captura últimos N chars antes do caret
// + N depois, monta prompt "complete o trecho mantendo estilo", envia ao
// provider configurado (anthropic/openai), pinta o resultado como
// Scintilla EOL annotation (estilo cinza). Tab no Normal mode insere o
// texto, Esc descarta. Persistência em QSettings/AiGhost/Enabled.
class AiGhostCompletion : public QObject {
    Q_OBJECT
public:
    explicit AiGhostCompletion(QObject* parent = nullptr);

    void setEnabled(bool on);
    bool isEnabled() const { return m_enabled; }

    void attach(ScintillaEdit* editor);
    void detach(ScintillaEdit* editor);

    // Tab handler invoked from MainWindow's eventFilter — returns true if a
    // ghost suggestion was visible and got accepted.
    bool acceptCurrent(ScintillaEdit* editor);

    // Esc handler — clears the ghost without accepting.
    bool dismissCurrent(ScintillaEdit* editor);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onIdleTimeout();

private:
    void scheduleRequest(ScintillaEdit* sci);
    void sendRequest(ScintillaEdit* sci, const QString& prefix, const QString& suffix);
    void renderGhost(ScintillaEdit* sci, const QString& text);
    void clearGhost(ScintillaEdit* sci);

    bool         m_enabled = false;
    QTimer*      m_idleTimer = nullptr;
    QPointer<ScintillaEdit> m_pending;     // editor that's waiting for a reply

    QNetworkAccessManager* m_nam = nullptr;
    QNetworkReply*         m_reply = nullptr;

    // Per-editor active ghost text + caret-at-fetch.
    struct Ghost {
        QString text;
        int     caretAtFetch = 0;
    };
    QHash<ScintillaEdit*, Ghost> m_ghosts;
};
