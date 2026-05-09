"""Ordena as linhas selecionadas (asc/desc, numérico ou lexicográfico).

Demonstra: substituição parcial de seleção, parse opcional de números.
"""
import re

import notepadpp


def _try_float(s: str):
    try:
        return float(s)
    except ValueError:
        return None


def _sort(reverse: bool, numeric: bool):
    sel = notepadpp.editor.selected_text()
    if not sel:
        notepadpp.ui.show_message("Selecione as linhas a ordenar.", "Sort")
        return
    lines = sel.splitlines()
    if numeric:
        # extrai 1º número de cada linha; linhas sem número vão pro fim
        def key(line):
            m = re.search(r"-?\d+(?:\.\d+)?", line)
            n = _try_float(m.group(0)) if m else None
            return (n is None, n if n is not None else 0, line)
        lines.sort(key=key, reverse=reverse)
    else:
        lines.sort(key=str.lower, reverse=reverse)
    notepadpp.editor.replace_selection("\n".join(lines))


notepadpp.ui.add_action("Plugins/Sort/Lex asc",     lambda: _sort(False, False))
notepadpp.ui.add_action("Plugins/Sort/Lex desc",    lambda: _sort(True,  False))
notepadpp.ui.add_action("Plugins/Sort/Numérico asc", lambda: _sort(False, True))
notepadpp.ui.add_action("Plugins/Sort/Numérico desc", lambda: _sort(True,  True))
