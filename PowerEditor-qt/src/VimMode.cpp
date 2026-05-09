#include "VimMode.h"

#include <QApplication>
#include <QClipboard>
#include <QEvent>
#include <QInputDialog>
#include <QKeyEvent>
#include <QSettings>

#include <ScintillaEdit.h>

namespace {
constexpr const char* kEnabledKey = "VimMode/Enabled";

bool isWordCharByte(unsigned char c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z') || c == '_'
        || c >= 0x80;   // permissive for UTF-8 continuation bytes
}
}

VimMode::VimMode(QObject* parent) : QObject(parent)
{
    QSettings s;
    m_enabled = s.value(kEnabledKey, false).toBool();
}

QString VimMode::modeLabel() const
{
    switch (m_mode) {
        case Mode::Normal: return QStringLiteral("VIM:NORMAL");
        case Mode::Insert: return QStringLiteral("VIM:INSERT");
        case Mode::Visual: return QStringLiteral("VIM:VISUAL");
    }
    return QStringLiteral("VIM");
}

void VimMode::setEnabled(bool on)
{
    if (m_enabled == on) return;
    m_enabled = on;
    QSettings s; s.setValue(kEnabledKey, on);
    if (on) enterMode(Mode::Normal);
    emit modeChanged(m_mode);
}

void VimMode::attach(ScintillaEdit* editor)
{
    if (!editor || m_editors.contains(editor)) return;
    m_editors.insert(editor);
    editor->installEventFilter(this);
    connect(editor, &QObject::destroyed, this, [this](QObject* obj) {
        m_editors.remove(reinterpret_cast<ScintillaEdit*>(obj));
    });
}

void VimMode::detach(ScintillaEdit* editor)
{
    if (!editor) return;
    editor->removeEventFilter(this);
    m_editors.remove(editor);
}

void VimMode::enterMode(Mode m)
{
    if (m_mode == m) { emit modeChanged(m); return; }
    m_mode = m;
    if (m != Mode::Visual) m_visualAnchor = -1;
    resetPending();
    emit modeChanged(m);
}

void VimMode::resetPending()
{
    m_pendingOp = 0;
    m_pendingCount = 0;
    m_gPending = false;
    emit commandPending(QString());
}

bool VimMode::eventFilter(QObject* watched, QEvent* event)
{
    if (!m_enabled) return QObject::eventFilter(watched, event);
    if (event->type() != QEvent::KeyPress) return QObject::eventFilter(watched, event);
    auto* sci = qobject_cast<ScintillaEdit*>(watched);
    if (!sci || !m_editors.contains(sci)) return QObject::eventFilter(watched, event);

    auto* ke = static_cast<QKeyEvent*>(event);
    if (handleKey(sci, ke)) return true;
    return QObject::eventFilter(watched, event);
}

bool VimMode::handleKey(ScintillaEdit* sci, QKeyEvent* event)
{
    // Esc ALWAYS goes to Normal mode and absorbs the keystroke.
    if (event->key() == Qt::Key_Escape) {
        enterMode(Mode::Normal);
        return true;
    }
    switch (m_mode) {
        case Mode::Insert:
            // In Insert mode we let the editor process the keystroke as usual.
            return false;
        case Mode::Normal: {
            bool consumed = false;
            handleNormal(sci, event, consumed);
            return consumed;
        }
        case Mode::Visual: {
            bool consumed = false;
            handleVisual(sci, event, consumed);
            return consumed;
        }
    }
    return false;
}

int VimMode::motionTarget(ScintillaEdit* sci, int from, char motion, int count)
{
    if (!sci) return -1;
    const int len = static_cast<int>(sci->length());
    int pos = from;

    switch (motion) {
        case 'h': {
            for (int i = 0; i < count; ++i) {
                if (pos > 0) pos = static_cast<int>(sci->positionBefore(pos));
            }
            return pos;
        }
        case 'l': {
            for (int i = 0; i < count; ++i) {
                if (pos < len) pos = static_cast<int>(sci->positionAfter(pos));
            }
            return pos;
        }
        case 'k': {
            for (int i = 0; i < count; ++i) {
                const int line = static_cast<int>(sci->lineFromPosition(pos));
                if (line == 0) break;
                const int col  = pos - static_cast<int>(sci->positionFromLine(line));
                const int prev = static_cast<int>(sci->positionFromLine(line - 1));
                pos = qMin(prev + col, static_cast<int>(sci->lineEndPosition(line - 1)));
            }
            return pos;
        }
        case 'j': {
            for (int i = 0; i < count; ++i) {
                const int line = static_cast<int>(sci->lineFromPosition(pos));
                const int last = static_cast<int>(sci->lineCount()) - 1;
                if (line == last) break;
                const int col  = pos - static_cast<int>(sci->positionFromLine(line));
                const int next = static_cast<int>(sci->positionFromLine(line + 1));
                pos = qMin(next + col, static_cast<int>(sci->lineEndPosition(line + 1)));
            }
            return pos;
        }
        case '0': {
            const int line = static_cast<int>(sci->lineFromPosition(pos));
            return static_cast<int>(sci->positionFromLine(line));
        }
        case '$': {
            const int line = static_cast<int>(sci->lineFromPosition(pos));
            return static_cast<int>(sci->lineEndPosition(line));
        }
        case '^': {
            const int line = static_cast<int>(sci->lineFromPosition(pos));
            int p = static_cast<int>(sci->positionFromLine(line));
            const int eol = static_cast<int>(sci->lineEndPosition(line));
            QByteArray l = sci->getLine(line);
            int i = 0;
            while (i < l.size() && (l[i] == ' ' || l[i] == '\t')) ++i;
            return qMin(p + i, eol);
        }
        case 'w': {
            for (int i = 0; i < count; ++i) {
                QByteArray buf = sci->textRange(pos, qMin(pos + 256, len));
                int j = 0;
                // Skip current word.
                while (j < buf.size() && isWordCharByte(buf[j])) ++j;
                // Skip whitespace/punct after.
                while (j < buf.size() && !isWordCharByte(buf[j]) && buf[j] != '\n') ++j;
                if (j == 0 && pos < len) ++j;   // ensure progress
                pos = qMin(pos + j, len);
            }
            return pos;
        }
        case 'b': {
            for (int i = 0; i < count; ++i) {
                if (pos == 0) break;
                int j = pos;
                // Step back over ws/punct.
                while (j > 0) {
                    QByteArray b = sci->textRange(j - 1, j);
                    if (b.isEmpty()) break;
                    if (isWordCharByte(b[0])) break;
                    --j;
                }
                // Step back over the word.
                while (j > 0) {
                    QByteArray b = sci->textRange(j - 1, j);
                    if (b.isEmpty() || !isWordCharByte(b[0])) break;
                    --j;
                }
                if (j == pos && j > 0) --j;
                pos = j;
            }
            return pos;
        }
        case 'e': {
            for (int i = 0; i < count; ++i) {
                int j = pos;
                // If we're inside whitespace, skip until a word starts.
                while (j < len) {
                    QByteArray b = sci->textRange(j, j + 1);
                    if (b.isEmpty() || isWordCharByte(b[0])) break;
                    ++j;
                }
                // Advance to last char of this word.
                while (j < len) {
                    QByteArray b = sci->textRange(j, j + 1);
                    if (b.isEmpty() || !isWordCharByte(b[0])) break;
                    ++j;
                }
                if (j > pos) --j;
                pos = j;
            }
            return pos;
        }
        case 'G': {
            // count==0 → bottom; otherwise go to absolute line `count` (1-based).
            const int total = static_cast<int>(sci->lineCount());
            const int line = (count <= 0) ? total - 1 : qMin(count - 1, total - 1);
            return static_cast<int>(sci->positionFromLine(line));
        }
        case 'g': {  // 'gg' — top of file
            return 0;
        }
    }
    return -1;
}

void VimMode::handleNormal(ScintillaEdit* sci, QKeyEvent* event, bool& consumed)
{
    consumed = true;   // default; we'll set false explicitly at the bottom for unhandled keys

    const QString text = event->text();
    const int key = event->key();
    const int caret = static_cast<int>(sci->currentPos());

    // Digit count accumulator (a leading 0 is the line-start motion, not a count).
    if (text.size() == 1 && text[0].isDigit()
        && !(text[0] == '0' && m_pendingCount == 0)) {
        m_pendingCount = m_pendingCount * 10 + text[0].digitValue();
        emit commandPending(QString::number(m_pendingCount));
        return;
    }

    // Awaiting motion after operator (d/y/c).
    if (m_pendingOp != 0) {
        char op = m_pendingOp;
        // Doubled operator → linewise (dd/yy/cc).
        if ((op == 'd' && key == Qt::Key_D) ||
            (op == 'y' && key == Qt::Key_Y) ||
            (op == 'c' && key == Qt::Key_C)) {
            const int line = static_cast<int>(sci->lineFromPosition(caret));
            const int from = static_cast<int>(sci->positionFromLine(line));
            const int to   = static_cast<int>(sci->positionFromLine(line + 1));
            applyOperator(sci, op, from, to, /*linewise*/ true);
            resetPending();
            return;
        }
        // 'g' modifier (e.g. dgg → not implemented; just gg goto top).
        if (key == Qt::Key_G && !m_gPending) {
            m_gPending = true;
            emit commandPending(QString(QLatin1Char(op)) + QStringLiteral("g"));
            return;
        }
        char motion = 0;
        if (m_gPending && key == Qt::Key_G) { motion = 'g'; m_gPending = false; }
        else if (text.size() == 1) motion = text[0].toLatin1();
        if (motion == 0) { resetPending(); return; }
        const int target = motionTarget(sci, caret, motion, effectiveCount());
        if (target < 0) { resetPending(); return; }
        const int from = qMin(caret, target);
        const int to   = qMax(caret, target);
        applyOperator(sci, op, from, to, /*linewise*/ false);
        resetPending();
        return;
    }

    // Operators
    if (key == Qt::Key_D || key == Qt::Key_Y || key == Qt::Key_C) {
        m_pendingOp = (key == Qt::Key_D) ? 'd' : (key == Qt::Key_Y) ? 'y' : 'c';
        emit commandPending(QString(QLatin1Char(m_pendingOp)));
        return;
    }

    // Single-key actions
    switch (key) {
        case Qt::Key_H: { sci->gotoPos(motionTarget(sci, caret, 'h', effectiveCount())); resetPending(); return; }
        case Qt::Key_J: { sci->gotoPos(motionTarget(sci, caret, 'j', effectiveCount())); resetPending(); return; }
        case Qt::Key_K: { sci->gotoPos(motionTarget(sci, caret, 'k', effectiveCount())); resetPending(); return; }
        case Qt::Key_L: { sci->gotoPos(motionTarget(sci, caret, 'l', effectiveCount())); resetPending(); return; }
        case Qt::Key_W: { sci->gotoPos(motionTarget(sci, caret, 'w', effectiveCount())); resetPending(); return; }
        case Qt::Key_B: { sci->gotoPos(motionTarget(sci, caret, 'b', effectiveCount())); resetPending(); return; }
        case Qt::Key_E: { sci->gotoPos(motionTarget(sci, caret, 'e', effectiveCount())); resetPending(); return; }
        case Qt::Key_0: { sci->gotoPos(motionTarget(sci, caret, '0', 1)); resetPending(); return; }
        case Qt::Key_Dollar:
        case Qt::Key_4: { if (event->modifiers() & Qt::ShiftModifier || key == Qt::Key_Dollar) {
            sci->gotoPos(motionTarget(sci, caret, '$', 1)); resetPending(); return; } break; }
        case Qt::Key_AsciiCircum:
        case Qt::Key_6: { if (event->modifiers() & Qt::ShiftModifier || key == Qt::Key_AsciiCircum) {
            sci->gotoPos(motionTarget(sci, caret, '^', 1)); resetPending(); return; } break; }
        case Qt::Key_G: {
            if (m_gPending) {
                sci->gotoPos(motionTarget(sci, caret, 'g', 1));
                m_gPending = false; resetPending(); return;
            }
            if (event->modifiers() & Qt::ShiftModifier) {
                // 'G' (capital) — go to last line or to count.
                sci->gotoPos(motionTarget(sci, caret, 'G', m_pendingCount));
                resetPending(); return;
            }
            m_gPending = true; emit commandPending(QStringLiteral("g")); return;
        }
        case Qt::Key_X: {
            // x — delete char under caret. count repeats.
            for (int i = 0; i < effectiveCount() && caret < sci->length(); ++i) {
                sci->deleteRange(sci->currentPos(), 1);
            }
            resetPending(); return;
        }
        case Qt::Key_P: {
            const bool after = !(event->modifiers() & Qt::ShiftModifier);
            doPaste(sci, after);
            resetPending(); return;
        }
        case Qt::Key_I: {
            if (event->modifiers() & Qt::ShiftModifier) {
                // 'I' — insert at first non-blank.
                sci->gotoPos(motionTarget(sci, caret, '^', 1));
            }
            enterMode(Mode::Insert); return;
        }
        case Qt::Key_A: {
            if (event->modifiers() & Qt::ShiftModifier) {
                sci->gotoPos(motionTarget(sci, caret, '$', 1));
            } else if (caret < sci->length()) {
                sci->gotoPos(static_cast<int>(sci->positionAfter(caret)));
            }
            enterMode(Mode::Insert); return;
        }
        case Qt::Key_O: {
            const int line = static_cast<int>(sci->lineFromPosition(caret));
            if (event->modifiers() & Qt::ShiftModifier) {
                sci->gotoPos(static_cast<int>(sci->positionFromLine(line)));
                sci->newLine();
                sci->gotoPos(static_cast<int>(sci->positionFromLine(line)));
            } else {
                sci->gotoPos(static_cast<int>(sci->lineEndPosition(line)));
                sci->newLine();
            }
            enterMode(Mode::Insert);
            return;
        }
        case Qt::Key_U: {
            sci->undo();
            resetPending(); return;
        }
        case Qt::Key_R: {
            if (event->modifiers() & Qt::ControlModifier) {
                sci->redo();
                resetPending(); return;
            }
            break;
        }
        case Qt::Key_V: {
            m_visualAnchor = caret;
            enterMode(Mode::Visual);
            return;
        }
        case Qt::Key_Colon:
        case Qt::Key_Semicolon: {
            // ':' may arrive as Shift+; on US layouts.
            if (key == Qt::Key_Colon || (event->modifiers() & Qt::ShiftModifier)) {
                bool ok = false;
                const QString cmd = QInputDialog::getText(sci, tr("Vim ex"),
                    QStringLiteral(":"), QLineEdit::Normal, QString(), &ok);
                if (ok && !cmd.isEmpty()) runExCommand(sci, cmd);
                resetPending();
                return;
            }
            break;
        }
        default: break;
    }

    consumed = false;
}

void VimMode::handleVisual(ScintillaEdit* sci, QKeyEvent* event, bool& consumed)
{
    consumed = true;

    auto extendSelection = [&](int caretTarget) {
        const int from = qMin(m_visualAnchor, caretTarget);
        const int to   = qMax(m_visualAnchor, caretTarget);
        sci->setSel(from, to);
    };

    const int caret = static_cast<int>(sci->currentPos());
    switch (event->key()) {
        case Qt::Key_H: extendSelection(motionTarget(sci, caret, 'h', 1)); return;
        case Qt::Key_J: extendSelection(motionTarget(sci, caret, 'j', 1)); return;
        case Qt::Key_K: extendSelection(motionTarget(sci, caret, 'k', 1)); return;
        case Qt::Key_L: extendSelection(motionTarget(sci, caret, 'l', 1)); return;
        case Qt::Key_W: extendSelection(motionTarget(sci, caret, 'w', 1)); return;
        case Qt::Key_B: extendSelection(motionTarget(sci, caret, 'b', 1)); return;
        case Qt::Key_0: extendSelection(motionTarget(sci, caret, '0', 1)); return;
        case Qt::Key_Dollar: extendSelection(motionTarget(sci, caret, '$', 1)); return;
        case Qt::Key_D:
        case Qt::Key_X:
        case Qt::Key_C:
        case Qt::Key_Y: {
            const int from = sci->selectionStart();
            const int to   = sci->selectionEnd();
            char op = (event->key() == Qt::Key_Y) ? 'y'
                    : (event->key() == Qt::Key_C) ? 'c'
                    : 'd';
            applyOperator(sci, op, from, to, /*linewise*/ false);
            enterMode(op == 'c' ? Mode::Insert : Mode::Normal);
            return;
        }
        default: break;
    }
    consumed = false;
}

void VimMode::applyOperator(ScintillaEdit* sci, char op, int from, int to, bool linewise)
{
    if (from >= to) return;
    if (op == 'y') {
        doYank(sci, from, to, linewise);
        return;
    }
    if (op == 'd' || op == 'c') {
        doYank(sci, from, to, linewise);
        doDelete(sci, from, to, linewise);
        if (op == 'c') enterMode(Mode::Insert);
    }
}

void VimMode::doYank(ScintillaEdit* sci, int from, int to, bool linewise)
{
    m_register = sci->textRange(from, to);
    m_registerLinewise = linewise;
    QApplication::clipboard()->setText(QString::fromUtf8(m_register));
}

void VimMode::doDelete(ScintillaEdit* sci, int from, int to, bool /*linewise*/)
{
    sci->beginUndoAction();
    sci->setTargetRange(from, to);
    sci->replaceTarget(0, "");
    sci->endUndoAction();
    sci->gotoPos(from);
}

void VimMode::doPaste(ScintillaEdit* sci, bool after)
{
    if (m_register.isEmpty()) {
        // Fall back to the system clipboard so vim-mode users still get a
        // useful paste even without a prior yank.
        m_register = QApplication::clipboard()->text().toUtf8();
        if (m_register.isEmpty()) return;
    }
    const int caret = static_cast<int>(sci->currentPos());
    int insertAt = caret;
    sci->beginUndoAction();
    if (m_registerLinewise) {
        const int line = static_cast<int>(sci->lineFromPosition(caret));
        insertAt = after
                       ? static_cast<int>(sci->positionFromLine(line + 1))
                       : static_cast<int>(sci->positionFromLine(line));
        sci->insertText(insertAt, m_register.constData());
    } else {
        if (after && caret < sci->length())
            insertAt = static_cast<int>(sci->positionAfter(caret));
        sci->insertText(insertAt, m_register.constData());
        sci->gotoPos(insertAt + m_register.size());
    }
    sci->endUndoAction();
}

void VimMode::runExCommand(ScintillaEdit* sci, const QString& cmd)
{
    // :w :q :wq :x :e <file> :s/old/new/[g]
    const QString trimmed = cmd.trimmed();
    if (trimmed == QStringLiteral("w"))      { emit requestSaveCurrentTab(); return; }
    if (trimmed == QStringLiteral("q"))      { emit requestCloseCurrentTab(); return; }
    if (trimmed == QStringLiteral("wq") || trimmed == QStringLiteral("x")) {
        emit requestSaveCurrentTab();
        emit requestCloseCurrentTab();
        return;
    }
    if (trimmed.startsWith(QStringLiteral("e "))) {
        emit requestOpenFile(trimmed.mid(2).trimmed());
        return;
    }
    if (trimmed.startsWith(QStringLiteral("s/"))) {
        // Naive substitution on the current line. Format: s/old/new/  with
        // optional trailing 'g' for global-on-line.
        const QString body = trimmed.mid(2);
        const QStringList parts = body.split(QLatin1Char('/'));
        if (parts.size() < 2) return;
        const QString needle = parts.value(0);
        const QString repl   = parts.value(1);
        const bool global    = parts.value(2).contains(QLatin1Char('g'));
        const int caret = static_cast<int>(sci->currentPos());
        const int line  = static_cast<int>(sci->lineFromPosition(caret));
        const int from  = static_cast<int>(sci->positionFromLine(line));
        const int to    = static_cast<int>(sci->lineEndPosition(line));
        QString text = QString::fromUtf8(sci->textRange(from, to));
        if (global) text.replace(needle, repl);
        else        text.replace(text.indexOf(needle), needle.size(), repl);
        sci->beginUndoAction();
        sci->setTargetRange(from, to);
        sci->replaceTarget(text.toUtf8().size(), text.toUtf8().constData());
        sci->endUndoAction();
        return;
    }
}
