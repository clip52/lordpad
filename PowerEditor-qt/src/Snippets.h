#pragma once
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QList>
#include <ScintillaTypes.h>

class ScintillaEdit;

struct Snippet {
    QString trigger;     // word the user types before pressing Tab (e.g. "for")
    QString body;        // expansion text; ${1}, ${2:default} placeholders
    QString description; // shown in the manager dialog
};

class Snippets : public QObject {
    Q_OBJECT
public:
    explicit Snippets(QObject* parent = nullptr);

    // Look up snippets for a specific Lexilla lexer name (e.g. "cpp", "python").
    // "" = global snippets that apply to any language.
    QList<Snippet> forLanguage(const QString& lexerName) const;

    // Add or update a snippet under the given language.
    void upsert(const QString& lexerName, const Snippet& s);
    void remove(const QString& lexerName, const QString& trigger);

    // All language buckets (sorted).
    QStringList allLanguages() const;

    // Try to expand the trigger word at the caret.
    // Returns true if expansion happened (caller should consume the Tab key event).
    // Logic:
    //   - get the word immediately before caret (word chars only)
    //   - look up snippet for current lexer name; if none, fall back to "" global bucket
    //   - if found: replace trigger word with body, parse ${N} / ${N:default}
    //     placeholders, start a navigation session and select the first stop.
    bool tryExpand(ScintillaEdit* editor, const QString& currentLexerName);

    // True iff there's an active multi-stop snippet session for `editor`.
    bool hasActiveSession(ScintillaEdit* editor) const;

    // Move to the next / previous tabstop in the active session. Returns true
    // when the navigation key was consumed (caller should swallow the event).
    bool advanceTabstop(ScintillaEdit* editor);
    bool retreatTabstop(ScintillaEdit* editor);

    // Cancel the active session immediately (e.g. on Escape).
    void cancelSession(ScintillaEdit* editor);

signals:
    void changed();

private slots:
    void onEditorModified(Scintilla::ModificationFlags type,
                          Scintilla::Position position,
                          Scintilla::Position length,
                          Scintilla::Position linesAdded,
                          const QByteArray& text,
                          Scintilla::Position line,
                          Scintilla::FoldLevel foldNow,
                          Scintilla::FoldLevel foldPrev);

private:
    void load();
    void save() const;
    void seedDefaults();

    // m_buckets[lexerName][trigger] -> Snippet.
    QHash<QString, QHash<QString, Snippet>> m_buckets;

    // ---- Active tabstop session ------------------------------------------
    struct Stop {
        int  number = 0;          // 1..K; 0 means "exit position"
        int  pos    = 0;          // absolute byte offset in the buffer
        int  len    = 0;          // bytes occupied by the placeholder default (0 if empty)
    };
    struct Session {
        QPointer<ScintillaEdit> editor;
        QList<Stop> stops;        // ordered by number (1, 2, ..., K, then 0)
        int         currentIndex = -1;
        // Bookkeeping for the modification listener below: when modifications
        // happen we shift `pos` for stops at or after the change point.
    };
    Session m_session;

    // Place caret at the active stop and select its default text. Returns
    // false (and ends the session) if the stop list is exhausted.
    bool selectStop(int index);

    void endSession();
};
