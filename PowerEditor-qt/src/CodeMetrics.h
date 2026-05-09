#pragma once

#include <QString>

// CodeMetrics (M20) — métricas básicas para o buffer ativo.
namespace CodeMetrics {

struct Result {
    int linesTotal = 0;
    int linesCode  = 0;     // não-blank, não-comment
    int linesComment = 0;
    int linesBlank   = 0;
    int functions = 0;      // estimado por padrões por linguagem
    int cyclomatic = 0;     // soma dos branches (if/for/while/case/&&/||/?:)
};

// Heurística leve por lexer name. Línguas conhecidas: cpp/c, python,
// javascript/typescript, java, rust, go. Outros caem no "genérico"
// que só conta linhas.
Result analyze(const QString& text, const QString& lexerName);

} // namespace CodeMetrics
