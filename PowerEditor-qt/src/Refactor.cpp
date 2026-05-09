#include "Refactor.h"

#include <QObject>
#include <QStringList>

#include <ScintillaEdit.h>

namespace Refactor {

namespace {

// Indent inferred from the first non-empty line of `text`. Returns the
// leading whitespace string ("" / "    " / "\t"). Used to keep the inserted
// definition at the same column as the call site.
QString leadingIndent(const QString& text) {
    for (const QString& line : text.split('\n')) {
        if (line.trimmed().isEmpty()) continue;
        QString lead;
        for (QChar c : line) {
            if (c == ' ' || c == '\t') lead.append(c);
            else break;
        }
        return lead;
    }
    return {};
}

// Re-indent every line of `body` to start at column 0 (strip the common
// leading prefix). Helps when extracting a deeply-nested block — the
// generated function shouldn't look like the body is double-indented.
QString deindent(const QString& body) {
    QStringList lines = body.split('\n');
    QString common;
    bool firstNonEmpty = true;
    for (const QString& line : lines) {
        if (line.trimmed().isEmpty()) continue;
        QString lead;
        for (QChar c : line) {
            if (c == ' ' || c == '\t') lead.append(c);
            else break;
        }
        if (firstNonEmpty) { common = lead; firstNonEmpty = false; }
        else {
            const int n = qMin(common.size(), lead.size());
            int i = 0;
            while (i < n && common[i] == lead[i]) ++i;
            common.truncate(i);
            if (common.isEmpty()) break;
        }
    }
    if (common.isEmpty()) return body;
    QStringList out;
    for (const QString& line : lines) {
        if (line.startsWith(common)) out.append(line.mid(common.size()));
        else                          out.append(line);
    }
    return out.join('\n');
}

// Re-add `prefix` to every non-empty line. Used when wrapping a function body
// for languages that require the body to be indented (Python).
QString reindent(const QString& body, const QString& prefix) {
    QStringList out;
    for (const QString& line : body.split('\n')) {
        if (line.isEmpty()) out.append(line);
        else                out.append(prefix + line);
    }
    return out.join('\n');
}

QString templateForLexer(const QString& lexer, const QString& name,
                         const QString& body, const QString& callIndent)
{
    const QString clean = deindent(body);
    if (lexer == QStringLiteral("python")) {
        return callIndent + QStringLiteral("def %1():\n").arg(name)
             + reindent(clean.endsWith('\n') ? clean.chopped(1) : clean, callIndent + QStringLiteral("    "))
             + QStringLiteral("\n\n");
    }
    if (lexer == QStringLiteral("javascript") || lexer == QStringLiteral("typescript")) {
        return callIndent + QStringLiteral("function %1() {\n").arg(name)
             + reindent(clean.endsWith('\n') ? clean.chopped(1) : clean, callIndent + QStringLiteral("    "))
             + QStringLiteral("\n") + callIndent + QStringLiteral("}\n\n");
    }
    if (lexer == QStringLiteral("cpp") || lexer == QStringLiteral("c")
        || lexer == QStringLiteral("rust") || lexer == QStringLiteral("go")) {
        const QString returnType = (lexer == QStringLiteral("rust")) ? QStringLiteral("fn ")
                                 : (lexer == QStringLiteral("go"))   ? QStringLiteral("func ")
                                 : QStringLiteral("void ");
        const QString suffix     = (lexer == QStringLiteral("rust")) ? QStringLiteral("()")
                                 : (lexer == QStringLiteral("go"))   ? QStringLiteral("()")
                                 : QStringLiteral("()");
        return callIndent + returnType + name + suffix + QStringLiteral(" {\n")
             + reindent(clean.endsWith('\n') ? clean.chopped(1) : clean, callIndent + QStringLiteral("    "))
             + QStringLiteral("\n") + callIndent + QStringLiteral("}\n\n");
    }
    // Fallback: generic text — emit a comment block above the call site so the
    // user sees a placeholder to fill in.
    return callIndent + QStringLiteral("// extracted: %1()\n").arg(name)
         + reindent(clean, callIndent)
         + QStringLiteral("\n");
}

QString callForLexer(const QString& lexer, const QString& name)
{
    if (lexer == QStringLiteral("python")) return name + QStringLiteral("()");
    return name + QStringLiteral("()");   // js/ts/cpp/c — same shape; ; appended at insert time
}

} // namespace

bool extractFunction(ScintillaEdit* editor, const QString& lexerName,
                     const QString& name, QString* outErr)
{
    if (!editor || name.trimmed().isEmpty()) {
        if (outErr) *outErr = QObject::tr("Nome da função vazio.");
        return false;
    }
    const sptr_t selStart = editor->selectionStart();
    const sptr_t selEnd   = editor->selectionEnd();
    if (selEnd <= selStart) {
        if (outErr) *outErr = QObject::tr("Selecione o bloco a extrair.");
        return false;
    }

    const QByteArray selBytes = editor->getSelText();
    const QString selText = QString::fromUtf8(selBytes);
    const QString indent  = leadingIndent(selText);

    // Find the line where the selection starts so we can place the new
    // function definition immediately above it.
    const sptr_t insertLine = editor->lineFromPosition(selStart);
    const sptr_t insertPos  = editor->positionFromLine(insertLine);

    QString call = callForLexer(lexerName, name);
    if (lexerName != QStringLiteral("python")
        && lexerName != QStringLiteral("rust")
        && lexerName != QStringLiteral("go")) {
        call += QStringLiteral(";");
    }

    const QString def = templateForLexer(lexerName, name, selText, indent);
    const QByteArray defBytes  = def.toUtf8();
    const QByteArray callBytes = (indent + call + QStringLiteral("\n")).toUtf8();

    editor->beginUndoAction();
    // Replace the selection with the call (preserves caret context).
    editor->setTargetRange(selStart, selEnd);
    editor->replaceTarget(callBytes.size(), callBytes.constData());
    // Insert the function definition above the line where the selection was.
    editor->insertText(insertPos, defBytes.constData());
    editor->endUndoAction();
    return true;
}

} // namespace Refactor
