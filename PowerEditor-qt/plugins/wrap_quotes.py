"""Envolve a seleção em aspas / parênteses / colchetes.

Demonstra: ui.add_action múltiplos com lambdas para parametrizar.
"""
import notepadpp


def _wrap(open_ch: str, close_ch: str):
    sel = notepadpp.editor.selected_text()
    if not sel:
        notepadpp.ui.show_message("Nenhuma seleção.", "Wrap")
        return
    notepadpp.editor.replace_selection(f"{open_ch}{sel}{close_ch}")


notepadpp.ui.add_action("Plugins/Surround/\"…\"",  lambda: _wrap('"', '"'))
notepadpp.ui.add_action("Plugins/Surround/'…'",    lambda: _wrap("'", "'"))
notepadpp.ui.add_action("Plugins/Surround/`…`",    lambda: _wrap('`', '`'))
notepadpp.ui.add_action("Plugins/Surround/(…)",    lambda: _wrap('(', ')'))
notepadpp.ui.add_action("Plugins/Surround/[…]",    lambda: _wrap('[', ']'))
notepadpp.ui.add_action("Plugins/Surround/{…}",    lambda: _wrap('{', '}'))
notepadpp.ui.add_action("Plugins/Surround/<…>",    lambda: _wrap('<', '>'))
notepadpp.ui.add_action("Plugins/Surround/«…»",    lambda: _wrap('«', '»'))
