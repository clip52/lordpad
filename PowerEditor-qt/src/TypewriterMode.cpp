#include "TypewriterMode.h"

#include <QSettings>

#include <ScintillaEdit.h>
#include <ScintillaTypes.h>

namespace {
constexpr const char* kEnabledKey = "TypewriterMode/Enabled";

// Caret policy bits (Scintilla SCI_SETYCARETPOLICY)
//   CARET_SLOP   = 0x01
//   CARET_STRICT = 0x04
//   CARET_EVEN   = 0x08
//   CARET_JUMPS  = 0x10
constexpr sptr_t kSlop   = 0x01;
constexpr sptr_t kStrict = 0x04;
constexpr sptr_t kEven   = 0x08;
}

TypewriterMode::TypewriterMode(QObject* parent) : QObject(parent)
{
    QSettings s;
    m_enabled = s.value(kEnabledKey, false).toBool();
}

void TypewriterMode::setEnabled(bool on)
{
    if (m_enabled == on) return;
    m_enabled = on;
    QSettings s; s.setValue(kEnabledKey, on);
    for (auto it = m_editors.begin(); it != m_editors.end(); ++it) {
        if (it.value()) applyTo(it.value().data());
    }
}

void TypewriterMode::attach(ScintillaEdit* editor)
{
    if (!editor) return;
    m_editors.insert(editor, QPointer<ScintillaEdit>(editor));
    applyTo(editor);
}

void TypewriterMode::detach(ScintillaEdit* editor)
{
    if (!editor) return;
    m_editors.remove(editor);
    // Restore default Scintilla scrolling on detach.
    editor->setYCaretPolicy(0, 0);
}

void TypewriterMode::applyTo(ScintillaEdit* editor) const
{
    if (!editor) return;
    if (m_enabled) {
        // CARET_SLOP|STRICT|EVEN with a slop = half the on-screen lines pins
        // the caret to the vertical middle. We sample linesOnScreen lazily so
        // the policy adapts to window resizes (Scintilla recomputes on the
        // next scroll anyway).
        const sptr_t lines = editor->linesOnScreen();
        const sptr_t slop  = qMax<sptr_t>(1, lines / 2);
        editor->setYCaretPolicy(kSlop | kStrict | kEven, slop);
        // Center current caret immediately.
        editor->scrollCaret();
    } else {
        editor->setYCaretPolicy(0, 0);
    }
}
