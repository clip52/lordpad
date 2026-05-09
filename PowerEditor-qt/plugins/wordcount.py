"""Word count completo (chars/words/lines) em pop-up.

Demonstra: editor.text() + editor.selected_text() + ui.show_message().
"""
import re

import notepadpp


def _count(text: str) -> dict:
    return {
        "chars":       len(text),
        "chars_no_ws": len(re.sub(r"\s+", "", text)),
        "words":       len(re.findall(r"\b\w+\b", text)),
        "lines":       text.count("\n") + (1 if text and not text.endswith("\n") else 0),
    }


def show_buffer_stats():
    stats = _count(notepadpp.editor.text())
    notepadpp.ui.show_message(
        f"Caracteres:        {stats['chars']}\n"
        f"  sem whitespace:  {stats['chars_no_ws']}\n"
        f"Palavras:          {stats['words']}\n"
        f"Linhas:            {stats['lines']}",
        "Word count — buffer",
    )


def show_selection_stats():
    sel = notepadpp.editor.selected_text()
    if not sel:
        notepadpp.ui.show_message("Nenhuma seleção.", "Word count")
        return
    stats = _count(sel)
    notepadpp.ui.show_message(
        f"Caracteres:        {stats['chars']}\n"
        f"  sem whitespace:  {stats['chars_no_ws']}\n"
        f"Palavras:          {stats['words']}\n"
        f"Linhas:            {stats['lines']}",
        "Word count — seleção",
    )


notepadpp.ui.add_action("Plugins/Word count/Buffer inteiro", show_buffer_stats)
notepadpp.ui.add_action("Plugins/Word count/Seleção",        show_selection_stats)
