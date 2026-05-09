"""Insere a data ISO atual no cursor (Plugins → Inserir data)."""
from datetime import datetime

import notepadpp


def insert_today():
    notepadpp.editor.insert(datetime.now().strftime("%Y-%m-%d"))


def insert_now():
    notepadpp.editor.insert(datetime.now().strftime("%Y-%m-%d %H:%M:%S"))


notepadpp.ui.add_action("Plugins/Data/Inserir data ISO",     insert_today)
notepadpp.ui.add_action("Plugins/Data/Inserir data e hora",  insert_now)
