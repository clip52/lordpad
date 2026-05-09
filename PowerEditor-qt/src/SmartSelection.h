#pragma once

#include <QString>

class ScintillaEdit;

// SmartSelection (M29) — operações sintáticas heurísticas sem LSP.
namespace SmartSelection {

// Expande a seleção pra próxima unidade lógica:
//   1) word → 2) WORD (run de não-espaço) → 3) bracket pair (paren/bracket/brace)
//      → 4) line(s) → 5) buffer inteiro.
// Retorna false quando já está no nível máximo.
bool expand(ScintillaEdit* editor);

// Encolhe um nível (memorizado como undo do expand quando possível).
bool shrink(ScintillaEdit* editor);

// Renomeia todas as ocorrências da palavra atual no escopo do bloco
// envolvente (delimitado por blank lines / { } / def / class). Retorna o
// número de substituições feitas.
int  renameInScope(ScintillaEdit* editor, const QString& newName);

} // namespace SmartSelection
