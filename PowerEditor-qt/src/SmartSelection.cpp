#include "SmartSelection.h"

#include <ScintillaEdit.h>

namespace SmartSelection {

namespace {

bool isWordByte(unsigned char c) {
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z') || c == '_' || c >= 0x80;
}

// Find the matching brace surrounding `pos`. Returns true and fills out
// (openPos, closePos) if found.
bool findEnclosingPair(ScintillaEdit* sci, int pos, int& openPos, int& closePos)
{
    const int len = static_cast<int>(sci->length());
    int depth = 0;
    int j = pos;
    char openCh = 0, closeCh = 0;
    // Scan backwards for an unmatched opener.
    while (j > 0) {
        const QByteArray b = sci->textRange(j - 1, j);
        if (b.isEmpty()) { --j; continue; }
        const char c = b[0];
        if (c == ')' || c == ']' || c == '}') ++depth;
        else if (c == '(' || c == '[' || c == '{') {
            if (depth == 0) { openCh = c;
                              closeCh = (c=='(')?')':((c=='[')?']':'}');
                              openPos = j - 1; break; }
            --depth;
        }
        --j;
    }
    if (openCh == 0) return false;
    // Scan forward for matching closer.
    depth = 1;
    int k = pos;
    while (k < len) {
        const QByteArray b = sci->textRange(k, k + 1);
        if (b.isEmpty()) break;
        const char c = b[0];
        if (c == openCh) ++depth;
        else if (c == closeCh) { --depth; if (depth == 0) { closePos = k + 1; return true; } }
        ++k;
    }
    return false;
}

}

bool expand(ScintillaEdit* editor)
{
    if (!editor) return false;
    const int len = static_cast<int>(editor->length());
    int s = static_cast<int>(editor->selectionStart());
    int e = static_cast<int>(editor->selectionEnd());

    auto setSel = [&](int a, int b) { editor->setSel(a, b); };

    // 1) No selection → word.
    if (s == e) {
        int ws = static_cast<int>(editor->wordStartPosition(s, true));
        int we = static_cast<int>(editor->wordEndPosition(s, true));
        if (ws < we) { setSel(ws, we); return true; }
    }

    // 2) Word → WORD (non-whitespace run).
    {
        bool isWord = true;
        for (int i = s; i < e; ++i) {
            const QByteArray b = editor->textRange(i, i + 1);
            if (b.isEmpty() || !isWordByte(b[0])) { isWord = false; break; }
        }
        if (isWord) {
            int ws = s, we = e;
            while (ws > 0) {
                const QByteArray b = editor->textRange(ws - 1, ws);
                if (b.isEmpty() || b[0] == ' ' || b[0] == '\t' || b[0] == '\n') break;
                --ws;
            }
            while (we < len) {
                const QByteArray b = editor->textRange(we, we + 1);
                if (b.isEmpty() || b[0] == ' ' || b[0] == '\t' || b[0] == '\n') break;
                ++we;
            }
            if (ws < s || we > e) { setSel(ws, we); return true; }
        }
    }

    // 3) Bracket pair around the selection.
    {
        int op = 0, cl = 0;
        if (findEnclosingPair(editor, s, op, cl)) {
            // Inside brackets first; if already inside, expand to include them.
            if (s > op + 1 || e < cl - 1) { setSel(op + 1, cl - 1); return true; }
            setSel(op, cl); return true;
        }
    }

    // 4) Whole line(s).
    const int ls = static_cast<int>(editor->positionFromLine(editor->lineFromPosition(s)));
    const int leLine = static_cast<int>(editor->lineFromPosition(e));
    const int le = static_cast<int>(editor->positionFromLine(leLine + 1));
    if (ls < s || le > e) { setSel(ls, le); return true; }

    // 5) Whole buffer.
    if (s > 0 || e < len) { setSel(0, len); return true; }
    return false;
}

bool shrink(ScintillaEdit* editor)
{
    if (!editor) return false;
    const int s = static_cast<int>(editor->selectionStart());
    const int e = static_cast<int>(editor->selectionEnd());
    if (s == e) return false;
    // Step 1: drop the trailing newline if there is one.
    if (e > s) {
        const QByteArray b = editor->textRange(e - 1, e);
        if (!b.isEmpty() && (b[0] == '\n' || b[0] == '\r')) { editor->setSel(s, e - 1); return true; }
    }
    // Otherwise collapse to caret at start of selection.
    editor->setSel(s, s);
    return true;
}

int renameInScope(ScintillaEdit* editor, const QString& newName)
{
    if (!editor) return 0;
    // Determine the word at caret.
    const int caret = static_cast<int>(editor->currentPos());
    const int wordStart = static_cast<int>(editor->wordStartPosition(caret, true));
    const int wordEnd   = static_cast<int>(editor->wordEndPosition(caret, true));
    if (wordStart >= wordEnd) return 0;
    const QString name = QString::fromUtf8(editor->textRange(wordStart, wordEnd));
    if (name.isEmpty()) return 0;

    // Determine scope: walk back/forward to nearest blank line, OR enclosing brace pair.
    int scopeStart = 0;
    int scopeEnd   = static_cast<int>(editor->length());
    int op = 0, cl = 0;
    if (findEnclosingPair(editor, caret, op, cl)) {
        scopeStart = op + 1;
        scopeEnd   = cl - 1;
    } else {
        // Blank-line based scope.
        const int line = static_cast<int>(editor->lineFromPosition(caret));
        int lineStart = line;
        while (lineStart > 0) {
            QByteArray l = editor->getLine(lineStart - 1);
            if (l.trimmed().isEmpty()) break;
            --lineStart;
        }
        int lineEnd = line;
        const int total = static_cast<int>(editor->lineCount());
        while (lineEnd < total - 1) {
            QByteArray l = editor->getLine(lineEnd + 1);
            if (l.trimmed().isEmpty()) break;
            ++lineEnd;
        }
        scopeStart = static_cast<int>(editor->positionFromLine(lineStart));
        scopeEnd   = static_cast<int>(editor->positionFromLine(lineEnd + 1));
    }

    QByteArray scopeText = editor->textRange(scopeStart, scopeEnd);
    QString s = QString::fromUtf8(scopeText);
    // Replace whole-word occurrences only.
    QString out;
    out.reserve(s.size());
    int count = 0;
    int i = 0;
    while (i < s.size()) {
        if (i + name.size() <= s.size() && s.mid(i, name.size()) == name) {
            const QChar before = (i == 0) ? QChar(' ') : s[i - 1];
            const QChar after  = (i + name.size() >= s.size()) ? QChar(' ') : s[i + name.size()];
            const auto isWordCh = [](QChar c) { return c.isLetterOrNumber() || c == '_'; };
            if (!isWordCh(before) && !isWordCh(after)) {
                out += newName;
                i += name.size();
                ++count;
                continue;
            }
        }
        out += s[i];
        ++i;
    }
    if (count == 0) return 0;
    QByteArray newBytes = out.toUtf8();
    editor->beginUndoAction();
    editor->setTargetRange(scopeStart, scopeEnd);
    editor->replaceTarget(newBytes.size(), newBytes.constData());
    editor->endUndoAction();
    return count;
}

} // namespace SmartSelection
