# LordPad — Plugins de exemplo

Esta pasta tem plugins Python que servem de referência para desenvolver os
seus próprios. Cada arquivo é um plugin independente.

## Como instalar

LordPad procura plugins em `~/.config/LordPad/plugins/*.py`. Para usar um
exemplo:

```bash
mkdir -p ~/.config/LordPad/plugins
cp plugins/examples/insert_date.py ~/.config/LordPad/plugins/
```

E reinicie o LordPad (ou abra o Plugin Manager pelo menu Plugins → Gerenciar
plugins…).

## API disponível (módulo `notepadpp`)

### `notepadpp.editor`

| Método                       | Retorna       | Descrição                              |
|------------------------------|---------------|----------------------------------------|
| `editor.text()`              | `str`         | Conteúdo completo do buffer ativo      |
| `editor.set_text(s)`         | —             | Sobrescreve buffer (preservando undo)  |
| `editor.insert(s)`           | —             | Insere texto na posição do cursor      |
| `editor.replace_selection(s)`| —             | Substitui seleção pelo texto           |
| `editor.selected_text()`     | `str`         | Seleção atual (string vazia se nada)   |
| `editor.cursor()`            | `(line, col)` | Linha e coluna do cursor (0-indexado)  |
| `editor.goto_line(n)`        | —             | Pula para linha n (0-indexado)         |
| `editor.line_count()`        | `int`         | Número total de linhas                 |
| `editor.line(n)`             | `str`         | Conteúdo da linha n                    |

### `notepadpp.buffer`

| Método                | Retorna   | Descrição                              |
|-----------------------|-----------|----------------------------------------|
| `buffer.path()`       | `str`     | Caminho absoluto (ou string vazia)     |
| `buffer.modified()`   | `bool`    | Se o buffer está sujo (não-salvo)      |
| `buffer.save()`       | `bool`    | Salva o buffer (False se sem path)     |
| `buffer.lexer()`      | `str`     | Nome do lexer ativo (cpp/python/…)     |

### `notepadpp.ui`

| Método                                  | Descrição                              |
|-----------------------------------------|----------------------------------------|
| `ui.show_message(text, title="Plugin")` | Pop-up de mensagem                     |
| `ui.input(prompt, default="")`          | Pede input do usuário, retorna `str`   |
| `ui.add_action(label, callback)`        | Adiciona entrada no menu Plugins       |

### Hooks (callbacks)

Registre uma função que será chamada em eventos do editor:

```python
import notepadpp

def my_handler():
    print("salvou!")

notepadpp.on_save(my_handler)         # após salvar buffer
notepadpp.on_load(my_handler)         # após carregar arquivo
notepadpp.on_text_changed(my_handler) # a cada edição (use com cuidado)
notepadpp.on_tab_changed(my_handler)  # ao trocar de tab/buffer ativo
```

## Estrutura de um plugin

Um plugin é apenas um `.py` que importa `notepadpp` e registra ações ou hooks
no nível de módulo (executados uma vez no `import`):

```python
"""Meu plugin."""
import notepadpp

def hello():
    notepadpp.ui.show_message("Olá do plugin!", "Saudação")

notepadpp.ui.add_action("Plugins/Meu plugin/Saudar", hello)
```

## Boas práticas

- Use docstrings — o Plugin Manager exibe a primeira linha como descrição.
- Não bloqueie a UI: para tarefas longas, dispare `subprocess` e use `on_save`
  ou um botão pra trigger manual.
- Não confie no estado do editor — sempre cheque `buffer.path()`,
  `editor.line_count()`, etc., antes de operar.
- Erros são logados no `stderr` do LordPad e exibidos no Plugin Manager.
