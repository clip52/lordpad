#pragma once

#include <QObject>
#include <QPointer>
#include <QHash>
#include <QTimer>

#include <ScintillaTypes.h>

class ScintillaEdit;

// BracketColors — colors matched (), [], {} pairs by nesting depth using
// Scintilla indicators 12..15 (4 colors cycling). Re-scans the buffer with
// a 200 ms debounce after every modification.
//
// Lexer-aware quoting: characters inside string/char literals (Scintilla
// styles in the SCE_*_STRING / SCE_*_CHARACTER family) are skipped — those
// can't be balanced against real code brackets.
class BracketColors : public QObject {
    Q_OBJECT
public:
    static constexpr int kIndicatorBase = 12;   // uses 12, 13, 14, 15

    explicit BracketColors(QObject* parent = nullptr);

    // Singleton accessor matching the Smart-/Multi-Cursor convention.
    static BracketColors& shared();

    // Attach the colorizer to a fresh ScintillaEdit. Idempotent — attaching
    // the same editor twice is a no-op.
    static void installFor(ScintillaEdit* editor);

    void attach(ScintillaEdit* editor);
    void detach(ScintillaEdit* editor);

    void setEnabled(bool on);
    bool isEnabled() const { return m_enabled; }

private slots:
    void onEditorDestroyed(QObject* obj);
    void onModified(Scintilla::ModificationFlags type,
                    Scintilla::Position position, Scintilla::Position length,
                    Scintilla::Position linesAdded, const QByteArray& text,
                    Scintilla::Position line, Scintilla::FoldLevel foldNow,
                    Scintilla::FoldLevel foldPrev);
    void onRescanTimeout();

private:
    struct Entry {
        QPointer<ScintillaEdit> editor;
        QTimer* debounce = nullptr;
    };

    void configureIndicators(ScintillaEdit* editor) const;
    void clearIndicators(ScintillaEdit* editor) const;
    void rescan(ScintillaEdit* editor) const;

    QHash<ScintillaEdit*, Entry> m_entries;
    bool m_enabled = true;
};
