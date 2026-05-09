"""Formata o buffer Python via `black` ou `ruff format` (subprocess).

Demonstra:
  - leitura do buffer, escrita do resultado de volta;
  - integração com ferramenta CLI externa (subprocess);
  - branch entre formatadores disponíveis.
"""
import shutil
import subprocess

import notepadpp


def _format_with(cmd: list[str]) -> tuple[bool, str]:
    """Roda `cmd` lendo buffer pelo stdin, devolve (ok, novo_texto_ou_erro)."""
    proc = subprocess.run(
        cmd,
        input=notepadpp.editor.text(),
        text=True,
        capture_output=True,
        timeout=15,
    )
    if proc.returncode != 0:
        return False, proc.stderr.strip() or "erro de formatação"
    return True, proc.stdout


def format_python():
    if shutil.which("black"):
        cmd = ["black", "-q", "--stdin-filename", "x.py", "-"]
    elif shutil.which("ruff"):
        cmd = ["ruff", "format", "--stdin-filename", "x.py", "-"]
    else:
        notepadpp.ui.show_message(
            "Instale `black` ou `ruff` para usar este plugin.", "format_python"
        )
        return
    ok, out = _format_with(cmd)
    if not ok:
        notepadpp.ui.show_message(out, "format_python — erro")
        return
    notepadpp.editor.set_text(out)


notepadpp.ui.add_action("Plugins/Format/Python (black/ruff)", format_python)
