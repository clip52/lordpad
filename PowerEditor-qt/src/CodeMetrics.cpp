#include "CodeMetrics.h"

#include <QRegularExpression>
#include <QStringList>

namespace CodeMetrics {

namespace {

bool isBlockCommentStart(const QString& line, int& outOffset) {
    outOffset = line.indexOf(QStringLiteral("/*"));
    return outOffset >= 0;
}
bool isBlockCommentEnd(const QString& line, int& outOffset) {
    outOffset = line.indexOf(QStringLiteral("*/"));
    return outOffset >= 0;
}

void countLines(const QStringList& lines, const QString& lexerName, Result& r)
{
    const bool cpp = (lexerName == QStringLiteral("cpp") || lexerName == QStringLiteral("c")
                  || lexerName == QStringLiteral("javascript") || lexerName == QStringLiteral("typescript")
                  || lexerName == QStringLiteral("java") || lexerName == QStringLiteral("rust")
                  || lexerName == QStringLiteral("go"));
    const QString lineComment = (lexerName == QStringLiteral("python")) ? QStringLiteral("#") : QStringLiteral("//");

    bool inBlock = false;
    for (const QString& raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty()) { ++r.linesBlank; continue; }
        if (cpp && inBlock) {
            ++r.linesComment;
            int p = -1;
            if (isBlockCommentEnd(line, p)) inBlock = false;
            continue;
        }
        if (cpp) {
            int p = -1;
            if (isBlockCommentStart(line, p)) {
                ++r.linesComment;
                int e = -1;
                if (!isBlockCommentEnd(line.mid(p + 2), e)) inBlock = true;
                continue;
            }
        }
        if (line.startsWith(lineComment)) { ++r.linesComment; continue; }
        ++r.linesCode;
    }
}

void countDecisions(const QString& text, const QString& lexerName, Result& r)
{
    // Cyclomatic ≈ 1 + count of decision points. We sum &&, ||, ?, and the
    // language's `if/for/while/case` keywords.
    static const QRegularExpression rxOps(QStringLiteral(R"(&&|\|\||\?)"));
    int decisions = 0;
    {
        auto it = rxOps.globalMatch(text);
        while (it.hasNext()) { it.next(); ++decisions; }
    }
    QStringList kws;
    if (lexerName == QStringLiteral("python"))
        kws = { QStringLiteral("\\bif\\b"), QStringLiteral("\\belif\\b"),
                QStringLiteral("\\bfor\\b"), QStringLiteral("\\bwhile\\b") };
    else
        kws = { QStringLiteral("\\bif\\b"), QStringLiteral("\\belse if\\b"),
                QStringLiteral("\\bfor\\b"), QStringLiteral("\\bwhile\\b"),
                QStringLiteral("\\bcase\\b"), QStringLiteral("\\bcatch\\b") };
    for (const QString& pat : kws) {
        QRegularExpression rx(pat);
        auto it = rx.globalMatch(text);
        while (it.hasNext()) { it.next(); ++decisions; }
    }
    r.cyclomatic = 1 + decisions;
}

void countFunctions(const QString& text, const QString& lexerName, Result& r)
{
    QStringList patterns;
    if (lexerName == QStringLiteral("python"))
        patterns << QStringLiteral(R"(^\s*def\s+\w+\s*\()");
    else if (lexerName == QStringLiteral("javascript") || lexerName == QStringLiteral("typescript"))
        patterns << QStringLiteral(R"(\bfunction\s+\w+\s*\()")
                 << QStringLiteral(R"((?:^|\s)(\w+)\s*=\s*\([^)]*\)\s*=>)");
    else if (lexerName == QStringLiteral("rust"))
        patterns << QStringLiteral(R"(^\s*(?:pub\s+)?fn\s+\w+\s*\()");
    else if (lexerName == QStringLiteral("go"))
        patterns << QStringLiteral(R"(^\s*func\s+(?:\([^)]*\)\s*)?\w+\s*\()");
    else
        // C/C++/Java approximation: <retval> name(args)... { at end of line
        patterns << QStringLiteral(R"(^[\w\s\*&:<>]+\s+(\w+)\s*\([^;]*\)\s*\{?\s*$)");

    int count = 0;
    for (const QString& pat : patterns) {
        QRegularExpression rx(pat, QRegularExpression::MultilineOption);
        auto it = rx.globalMatch(text);
        while (it.hasNext()) { it.next(); ++count; }
    }
    r.functions = count;
}

}

Result analyze(const QString& text, const QString& lexerName)
{
    Result r;
    const QStringList lines = text.split('\n');
    r.linesTotal = lines.size();
    countLines(lines, lexerName, r);
    countDecisions(text, lexerName, r);
    countFunctions(text, lexerName, r);
    return r;
}

} // namespace CodeMetrics
