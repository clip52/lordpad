#pragma once

#include <QString>
#include <QHash>

// EditorConfig (M17) — minimal .editorconfig reader.
//
// Walks up from a file's directory until it finds a `.editorconfig` (or one
// marked `root = true`), parses the matching glob sections, and returns the
// merged settings. We honor: indent_style, indent_size, tab_width,
// end_of_line, charset, trim_trailing_whitespace, insert_final_newline.
namespace EditorConfig {

struct Settings {
    QString indent_style;     // "space" | "tab"
    int     indent_size = -1;
    int     tab_width   = -1;
    QString end_of_line;      // "lf" | "crlf" | "cr"
    QString charset;          // "utf-8" | "latin1" | ...
    int     trim_trailing_whitespace = -1;   // 0 / 1 / -1=unset
    int     insert_final_newline     = -1;
};

Settings settingsFor(const QString& filePath);

} // namespace EditorConfig
