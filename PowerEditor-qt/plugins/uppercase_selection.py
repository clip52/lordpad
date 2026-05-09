"""Transforma a seleção em CAIXA ALTA (Plugins → Texto → Uppercase)."""
import notepadpp


def uppercase():
    sel = notepadpp.editor.selected_text()
    if not sel:
        notepadpp.ui.show_message("Nenhuma seleção.", "Uppercase")
        return
    notepadpp.editor.replace_selection(sel.upper())


def lowercase():
    sel = notepadpp.editor.selected_text()
    if not sel:
        notepadpp.ui.show_message("Nenhuma seleção.", "Lowercase")
        return
    notepadpp.editor.replace_selection(sel.lower())


def title_case():
    sel = notepadpp.editor.selected_text()
    if not sel:
        notepadpp.ui.show_message("Nenhuma seleção.", "Title case")
        return
    notepadpp.editor.replace_selection(sel.title())


notepadpp.ui.add_action("Plugins/Texto/UPPERCASE",  uppercase)
notepadpp.ui.add_action("Plugins/Texto/lowercase",  lowercase)
notepadpp.ui.add_action("Plugins/Texto/Title Case", title_case)
