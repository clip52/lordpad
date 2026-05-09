#pragma once

#include <QObject>
#include <QSet>
#include <QPointer>

class ScintillaEdit;
class QKeyEvent;

// AutoPair (M17) — auto-closes brackets/quotes and "wraps" the selection
// when an opener is typed with text already selected. Persisted toggle in
// QSettings/AutoPair/Enabled. Skip-on-overtype: typing the closer when the
// next char is the same closer just moves the caret over it.
class AutoPair : public QObject {
    Q_OBJECT
public:
    explicit AutoPair(QObject* parent = nullptr);

    void setEnabled(bool on);
    bool isEnabled() const { return m_enabled; }

    void attach(ScintillaEdit* editor);
    void detach(ScintillaEdit* editor);

    // Surround the current selection with `open`/`close`. Returns false if
    // there is no selection.
    bool surround(ScintillaEdit* editor, QChar open, QChar close);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    bool handleKeyPress(ScintillaEdit* sci, QKeyEvent* event);

    bool m_enabled = true;
    QSet<ScintillaEdit*> m_editors;
};
