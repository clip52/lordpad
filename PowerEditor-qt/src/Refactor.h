#pragma once

#include <QString>

class ScintillaEdit;

// Refactor (M14) — small set of regex/text-driven refactorings that don't
// require LSP. They operate on the current selection in `editor` and respect
// the active lexer name to pick a sensible function-shape template.
namespace Refactor {

// Extract the current selection into a function `name()` and replace it with
// a call. Templates per lexer:
//   python       def NAME():\n<indented body>
//   cpp / c      void NAME() { <body> }
//   javascript   function NAME() { <body> }
//   default      ${lexer}-shaped fallback that prepends a comment + call.
//
// Returns false (and fills *outErr) when there is no selection or the
// language is unsupported.
bool extractFunction(ScintillaEdit* editor,
                     const QString& lexerName,
                     const QString& name,
                     QString* outErr = nullptr);

} // namespace Refactor
