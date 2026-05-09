#include "EditorConfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

namespace EditorConfig {

namespace {

// Convert an editorconfig glob to a regex. Subset enough to match the common
// patterns: *, **, ?, [seq], {a,b,c}.
QString globToRegex(const QString& glob) {
    QString out = QStringLiteral("^");
    int i = 0, n = glob.size();
    while (i < n) {
        const QChar c = glob[i];
        if (c == '*') {
            if (i + 1 < n && glob[i + 1] == '*') { out += QStringLiteral(".*"); i += 2; continue; }
            out += QStringLiteral("[^/]*"); ++i; continue;
        }
        if (c == '?') { out += QStringLiteral("[^/]"); ++i; continue; }
        if (c == '{') {
            int end = glob.indexOf('}', i + 1);
            if (end > i) {
                QString alts = glob.mid(i + 1, end - i - 1);
                QStringList parts = alts.split(',');
                for (QString& p : parts) p = QRegularExpression::escape(p);
                out += QStringLiteral("(?:") + parts.join(QStringLiteral("|")) + QStringLiteral(")");
                i = end + 1; continue;
            }
        }
        if (c == '[') {
            int end = glob.indexOf(']', i + 1);
            if (end > i) { out += glob.mid(i, end - i + 1); i = end + 1; continue; }
        }
        // Escape anything else regex-special.
        out += QRegularExpression::escape(c);
        ++i;
    }
    out += QStringLiteral("$");
    return out;
}

void applyKv(Settings& s, const QString& k, const QString& v) {
    if      (k == QStringLiteral("indent_style"))            s.indent_style = v;
    else if (k == QStringLiteral("indent_size"))             s.indent_size  = v.toInt();
    else if (k == QStringLiteral("tab_width"))               s.tab_width    = v.toInt();
    else if (k == QStringLiteral("end_of_line"))             s.end_of_line  = v;
    else if (k == QStringLiteral("charset"))                 s.charset      = v;
    else if (k == QStringLiteral("trim_trailing_whitespace"))s.trim_trailing_whitespace = (v == QStringLiteral("true")) ? 1 : 0;
    else if (k == QStringLiteral("insert_final_newline"))    s.insert_final_newline     = (v == QStringLiteral("true")) ? 1 : 0;
}

bool readFileInto(Settings& s, const QString& configPath, const QString& targetPath, bool& isRoot)
{
    QFile f(configPath); if (!f.open(QIODevice::ReadOnly)) return false;
    const QString configDir = QFileInfo(configPath).absolutePath();
    const QString rel = QDir(configDir).relativeFilePath(targetPath);

    QTextStream in(&f);
    QString currentSection;
    QRegularExpression rx;
    bool currentMatches = false;
    bool inGlobal = true;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#') || line.startsWith(';')) continue;
        if (line.startsWith('[') && line.endsWith(']')) {
            currentSection = line.mid(1, line.size() - 2);
            inGlobal = false;
            // Resolve glob: editorconfig globs without `/` match in any subdir;
            // with `/` they're rooted at the config file's dir.
            QString g = currentSection.contains('/')
                            ? currentSection
                            : QStringLiteral("**/") + currentSection;
            rx.setPattern(globToRegex(g));
            currentMatches = rx.match(rel).hasMatch();
            continue;
        }
        const int eq = line.indexOf('=');
        if (eq <= 0) continue;
        const QString k = line.left(eq).trimmed().toLower();
        const QString v = line.mid(eq + 1).trimmed().toLower();
        if (inGlobal) {
            if (k == QStringLiteral("root") && v == QStringLiteral("true")) isRoot = true;
            continue;
        }
        if (currentMatches) applyKv(s, k, v);
    }
    f.close();
    return true;
}

} // namespace

Settings settingsFor(const QString& filePath)
{
    Settings s;
    if (filePath.isEmpty()) return s;
    // Walk from filePath's directory upward, accumulating settings. Stop at a
    // root=true file or filesystem root.
    QDir d = QFileInfo(filePath).absoluteDir();
    bool stopped = false;
    int depth = 0;
    while (!stopped && depth < 32) {
        const QString cfg = d.absoluteFilePath(QStringLiteral(".editorconfig"));
        if (QFile::exists(cfg)) {
            bool isRoot = false;
            readFileInto(s, cfg, filePath, isRoot);
            if (isRoot) stopped = true;
        }
        if (!d.cdUp()) break;
        ++depth;
    }
    return s;
}

} // namespace EditorConfig
