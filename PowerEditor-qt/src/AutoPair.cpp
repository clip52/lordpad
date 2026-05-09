#include "AutoPair.h"

#include <QEvent>
#include <QKeyEvent>
#include <QSettings>

#include <ScintillaEdit.h>

namespace {
constexpr const char* kEnabledKey = "AutoPair/Enabled";

// Pairs we auto-close. Typing the opener inserts both and parks the caret
// in the middle. Quotes also self-close.
struct Pair { char open; char close; };
constexpr Pair kPairs[] = {
    { '(', ')' }, { '[', ']' }, { '{', '}' },
    { '"', '"' }, { '\'', '\'' }, { '`', '`' },
};

bool isCloser(char c) {
    return c == ')' || c == ']' || c == '}' || c == '"' || c == '\'' || c == '`';
}
}

AutoPair::AutoPair(QObject* parent) : QObject(parent)
{
    QSettings s; m_enabled = s.value(kEnabledKey, true).toBool();
}

void AutoPair::setEnabled(bool on)
{
    if (m_enabled == on) return;
    m_enabled = on;
    QSettings s; s.setValue(kEnabledKey, on);
}

void AutoPair::attach(ScintillaEdit* editor)
{
    if (!editor || m_editors.contains(editor)) return;
    m_editors.insert(editor);
    editor->installEventFilter(this);
    connect(editor, &QObject::destroyed, this, [this](QObject* obj) {
        m_editors.remove(reinterpret_cast<ScintillaEdit*>(obj));
    });
}

void AutoPair::detach(ScintillaEdit* editor)
{
    if (!editor) return;
    editor->removeEventFilter(this);
    m_editors.remove(editor);
}

bool AutoPair::eventFilter(QObject* watched, QEvent* event)
{
    if (!m_enabled) return QObject::eventFilter(watched, event);
    if (event->type() != QEvent::KeyPress) return QObject::eventFilter(watched, event);
    auto* sci = qobject_cast<ScintillaEdit*>(watched);
    if (!sci || !m_editors.contains(sci)) return QObject::eventFilter(watched, event);
    if (handleKeyPress(sci, static_cast<QKeyEvent*>(event))) return true;
    return QObject::eventFilter(watched, event);
}

bool AutoPair::handleKeyPress(ScintillaEdit* sci, QKeyEvent* event)
{
    const QString text = event->text();
    if (text.size() != 1) return false;
    const char c = text[0].toLatin1();
    if (c == 0) return false;

    // 1) Wrap selection when an opener is typed.
    if (sci->selectionStart() != sci->selectionEnd()) {
        for (const Pair& p : kPairs) {
            if (c == p.open) {
                surround(sci, QChar(p.open), QChar(p.close));
                return true;
            }
        }
    }

    // 2) Skip-on-overtype: typing a closer when next char is the same closer
    // just steps the caret over it.
    if (isCloser(c)) {
        const sptr_t pos = sci->currentPos();
        if (pos < sci->length()) {
            QByteArray nx = sci->textRange(pos, pos + 1);
            if (!nx.isEmpty() && nx[0] == c) {
                sci->gotoPos(pos + 1);
                return true;
            }
        }
    }

    // 3) Auto-close on opener.
    for (const Pair& p : kPairs) {
        if (c != p.open) continue;
        // Skip if the previous char is a word char and the opener is a quote
        // — avoids breaking apostrophes in English text.
        if (p.open == '\'' || p.open == '"' || p.open == '`') {
            const sptr_t pos = sci->currentPos();
            if (pos > 0) {
                QByteArray pv = sci->textRange(pos - 1, pos);
                if (!pv.isEmpty()) {
                    const unsigned char b = pv[0];
                    if ((b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z') || (b >= '0' && b <= '9'))
                        return false;
                }
            }
        }
        sci->beginUndoAction();
        const sptr_t pos = sci->currentPos();
        char buf[3] = { p.open, p.close, 0 };
        sci->insertText(pos, buf);
        sci->gotoPos(pos + 1);
        sci->endUndoAction();
        return true;
    }
    return false;
}

bool AutoPair::surround(ScintillaEdit* editor, QChar open, QChar close)
{
    if (!editor) return false;
    const sptr_t s = editor->selectionStart();
    const sptr_t e = editor->selectionEnd();
    if (s == e) return false;
    QByteArray sel = editor->textRange(s, e);
    QByteArray wrapped;
    wrapped.append(open.toLatin1());
    wrapped.append(sel);
    wrapped.append(close.toLatin1());
    editor->beginUndoAction();
    editor->setTargetRange(s, e);
    editor->replaceTarget(wrapped.size(), wrapped.constData());
    editor->setSel(s + 1, s + 1 + sel.size());
    editor->endUndoAction();
    return true;
}
