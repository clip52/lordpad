#pragma once

#include <QObject>

#include <ScintillaTypes.h>

#include "ScintillaEdit.h"

// MultiCursor — habilita multi-seleção / multi-cursor no Scintilla
// e oferece ações públicas auxiliares (Ctrl+Alt+Up/Down, Ctrl+Shift+L, Ctrl+D).
//
// Uso:
//   MultiCursor::installFor(editor);              // habilita flags + filtro de eventos
//   MultiCursor::addCursorBelow(editor);          // Ctrl+Alt+Down
//   MultiCursor::addCursorAbove(editor);          // Ctrl+Alt+Up
//   MultiCursor::selectAllOccurrences(editor);    // Ctrl+Shift+L
//   MultiCursor::addNextOccurrence(editor);       // Ctrl+D
class MultiCursor : public QObject {
    Q_OBJECT
public:
    explicit MultiCursor(QObject* parent = nullptr);

    // Configura os flags de multi-seleção e instala um filtro de eventos
    // no viewport para suporte a Ctrl+Click (adicionar caret) e Alt+drag
    // (seleção retangular). Seguro chamar várias vezes para o mesmo editor.
    static void installFor(ScintillaEdit* editor);

    // Adiciona um caret na linha imediatamente abaixo da seleção principal,
    // mantendo a mesma coluna visual.
    static void addCursorBelow(ScintillaEdit* editor);

    // Adiciona um caret na linha imediatamente acima da seleção principal,
    // mantendo a mesma coluna visual.
    static void addCursorAbove(ScintillaEdit* editor);

    // Seleciona todas as ocorrências da palavra ou seleção atual
    // (case-sensitive, palavra inteira quando não há seleção explícita).
    static void selectAllOccurrences(ScintillaEdit* editor);

    // Adiciona a próxima ocorrência da palavra/seleção atual ao multi-select.
    // A primeira invocação (sem seleção) seleciona apenas a palavra atual.
    static void addNextOccurrence(ScintillaEdit* editor);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
};
