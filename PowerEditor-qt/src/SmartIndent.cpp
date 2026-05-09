#include "SmartIndent.h"

#include <QEvent>
#include <QKeyEvent>
#include <QSettings>

#include <ScintillaEdit.h>

namespace {
constexpr const char* kEnabledKey = "SmartIndent/Enabled";

QByteArray indentForLine(ScintillaEdit* sci, int line) {
    QByteArray src = sci->getLine(line);
    int i = 0;
    while (i < src.size() && (src[i] == ' ' || src[i] == '\t')) ++i;
    return src.left(i);
}
}

SmartIndent::SmartIndent(QObject* parent) : QObject(parent)
{
    QSettings s; m_enabled = s.value(kEnabledKey, true).toBool();
}

void SmartIndent::setEnabled(bool on)
{
    if (m_enabled == on) return;
    m_enabled = on;
    QSettings s; s.setValue(kEnabledKey, on);
}

void SmartIndent::attach(ScintillaEdit* editor)
{
    if (!editor || m_editors.contains(editor)) return;
    m_editors.insert(editor);
    editor->installEventFilter(this);
    connect(editor, &QObject::destroyed, this, [this](QObject* obj) {
        m_editors.remove(reinterpret_cast<ScintillaEdit*>(obj));
    });
}
void SmartIndent::detach(ScintillaEdit* editor)
{
    if (!editor) return;
    editor->removeEventFilter(this);
    m_editors.remove(editor);
}

bool SmartIndent::eventFilter(QObject* watched, QEvent* event)
{
    if (!m_enabled) return QObject::eventFilter(watched, event);
    if (event->type() != QEvent::KeyPress) return QObject::eventFilter(watched, event);
    auto* sci = qobject_cast<ScintillaEdit*>(watched);
    if (!sci || !m_editors.contains(sci)) return QObject::eventFilter(watched, event);
    auto* ke = static_cast<QKeyEvent*>(event);
    if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
        if (handleEnter(sci)) return true;
    }
    if (ke->text() == QStringLiteral("}")) {
        if (handleClosingBrace(sci)) return true;
    }
    return QObject::eventFilter(watched, event);
}

bool SmartIndent::handleEnter(ScintillaEdit* sci)
{
    const sptr_t pos = sci->currentPos();
    const int line = static_cast<int>(sci->lineFromPosition(pos));
    QByteArray indent = indentForLine(sci, line);

    // Look at the last non-space character of the current line up to the caret.
    QByteArray lineText = sci->getLine(line);
    int upTo = pos - sci->positionFromLine(line);
    upTo = qMin(upTo, static_cast<int>(lineText.size()));
    int j = upTo - 1;
    while (j >= 0 && (lineText[j] == ' ' || lineText[j] == '\t')) --j;
    char lastCh = (j >= 0) ? lineText[j] : 0;
    bool extraIndent = (lastCh == '{' || lastCh == '(' || lastCh == '[' || lastCh == ':');

    QByteArray newIndent = indent;
    if (extraIndent) {
        // Match Scintilla's setting: tabs or N spaces.
        const bool useTabs = sci->useTabs();
        if (useTabs) newIndent += '\t';
        else         newIndent += QByteArray(qMax<int>(1, sci->tabWidth()), ' ');
    }
    sci->beginUndoAction();
    sci->newLine();
    // Append the indent immediately after the newline.
    sci->insertText(sci->currentPos(), newIndent.constData());
    sci->gotoPos(sci->currentPos() + newIndent.size());
    sci->endUndoAction();
    return true;
}

bool SmartIndent::handleClosingBrace(ScintillaEdit* sci)
{
    // Only auto-dedent when the caret is at the first non-space position on
    // the line — otherwise the user just wants a literal '}' inserted.
    const sptr_t pos = sci->currentPos();
    const int line = static_cast<int>(sci->lineFromPosition(pos));
    const sptr_t lineStart = sci->positionFromLine(line);
    QByteArray cur = sci->getLine(line);
    int leadEnd = 0;
    while (leadEnd < cur.size() && (cur[leadEnd] == ' ' || cur[leadEnd] == '\t')) ++leadEnd;
    if (pos != lineStart + leadEnd) return false;

    if (line == 0) return false;
    QByteArray prevIndent = indentForLine(sci, line - 1);
    // Drop one level: pop a trailing tab or `tabWidth` spaces.
    if (!prevIndent.isEmpty() && prevIndent.endsWith('\t')) prevIndent.chop(1);
    else {
        int n = qMax<int>(1, sci->tabWidth());
        while (n-- > 0 && !prevIndent.isEmpty() && prevIndent.endsWith(' ')) prevIndent.chop(1);
    }
    sci->beginUndoAction();
    sci->setTargetRange(lineStart, lineStart + leadEnd);
    sci->replaceTarget(prevIndent.size(), prevIndent.constData());
    // Now insert the brace at the new caret position.
    const sptr_t after = sci->currentPos();
    sci->insertText(after, "}");
    sci->gotoPos(after + 1);
    sci->endUndoAction();
    return true;
}
