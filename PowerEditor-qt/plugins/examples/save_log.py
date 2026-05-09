"""Loga todo save em ~/.cache/LordPad/save.log com timestamp.

Demonstra: hook on_save, leitura de buffer.path(), persistência fora do app.
"""
import os
from datetime import datetime
from pathlib import Path

import notepadpp


_LOG = Path.home() / ".cache" / "LordPad" / "save.log"


def _ensure_log_dir():
    _LOG.parent.mkdir(parents=True, exist_ok=True)


def on_save():
    _ensure_log_dir()
    path = notepadpp.buffer.path() or "<sem path>"
    ts = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    with _LOG.open("a", encoding="utf-8") as f:
        f.write(f"[{ts}] saved {path}\n")


def show_log():
    if not _LOG.exists():
        notepadpp.ui.show_message("Nenhum save ainda.", "Save log")
        return
    text = _LOG.read_text(encoding="utf-8", errors="replace")
    # Mostra só as últimas 30 linhas.
    tail = "\n".join(text.splitlines()[-30:])
    notepadpp.ui.show_message(tail, f"Save log ({_LOG})")


notepadpp.on_save(on_save)
notepadpp.ui.add_action("Plugins/Save log/Mostrar últimas 30 linhas", show_log)
