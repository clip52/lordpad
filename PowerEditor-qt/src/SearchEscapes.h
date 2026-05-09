#pragma once

#include <QString>

// SearchEscapes — Notepad++ "Extended" mode escape decoding. Used by both
// FindReplaceDialog and FindInFilesDialog so Find / Replace strings can
// contain literal escape sequences without enabling full regex.
//
// Recognized escapes (ASCII only):
//   \n  \r  \t  \0  \b  \f  \v  \\  \"  \'
//   \xHH       hex byte (1–2 digits)
//   \dDDD      decimal byte (1–3 digits, 0–255)
//   \oOOO      octal byte (1–3 digits)
//
// Unknown escapes pass through verbatim (the backslash is kept). This matches
// Notepad++'s tolerant behaviour and avoids surprises when users mix Extended
// mode with already-escaped paste-ins.
namespace SearchEscapes {

QString expandExtended(const QString& input);

} // namespace SearchEscapes
