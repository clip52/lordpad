"""Remove whitespace no fim das linhas, automaticamente ao salvar.

Demonstra: hook on_save + edit do buffer inteiro preservando undo via
editor.set_text().
"""
import notepadpp


def trim_on_save():
    text = notepadpp.editor.text()
    trimmed = "\n".join(line.rstrip() for line in text.split("\n"))
    if trimmed != text:
        notepadpp.editor.set_text(trimmed)


# Registra: chamado depois que LordPad salva o buffer.
notepadpp.on_save(trim_on_save)
