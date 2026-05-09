#pragma once

#include <QObject>
#include <QPointer>
#include <QHash>

class ScintillaEdit;

// TypewriterMode (M14) — keeps the caret near the vertical center of the
// viewport. Implemented purely with Scintilla's caret policy:
//   SCI_SETYCARETPOLICY(CARET_SLOP|CARET_STRICT|CARET_EVEN, slop)
// where slop is half the visible-line count → caret centered with a small
// dead band so micro-movements don't shake the screen.
//
// Persisted in QSettings ("TypewriterMode/Enabled"). Toggling re-applies the
// policy (or restores the default when off) on every attached editor.
class TypewriterMode : public QObject {
    Q_OBJECT
public:
    explicit TypewriterMode(QObject* parent = nullptr);

    void attach(ScintillaEdit* editor);
    void detach(ScintillaEdit* editor);

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool on);

private:
    void applyTo(ScintillaEdit* editor) const;

    bool m_enabled = false;
    QHash<ScintillaEdit*, QPointer<ScintillaEdit>> m_editors;
};
