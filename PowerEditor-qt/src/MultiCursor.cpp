#include "MultiCursor.h"

#include <QAbstractScrollArea>
#include <QByteArray>
#include <QEvent>
#include <QMouseEvent>
#include <QPointer>
#include <QVariant>

#include <Scintilla.h>

namespace {

// Marcador para garantir que o filtro só é instalado uma vez por editor.
constexpr const char* kInstalledProperty = "_multicursor_installed";

// Singleton lazy: um único QObject filtra eventos de todos os viewports.
// Vive dentro do parent do primeiro editor que o solicita; a destruição
// do parent (MainWindow) limpa o filtro automaticamente.
MultiCursor* sharedFilter() {
    static QPointer<MultiCursor> instance;
    if (!instance) {
        instance = new MultiCursor(nullptr);
    }
    return instance.data();
}

// Aplica os flags de multi-seleção descritos no Scintilla docs.
void applyMultiSelectionFlags(ScintillaEdit* editor) {
    editor->send(SCI_SETMULTIPLESELECTION, 1, 0);
    editor->send(SCI_SETADDITIONALSELECTIONTYPING, 1, 0);
    editor->send(SCI_SETADDITIONALCARETSBLINK, 1, 0);
    editor->send(SCI_SETADDITIONALCARETSVISIBLE, 1, 0);
    editor->send(SCI_SETMULTIPASTE, SC_MULTIPASTE_EACH, 0);
    editor->send(SCI_SETMOUSESELECTIONRECTANGULARSWITCH, 1, 0);
}

// Determina o intervalo de busca: seleção atual ou palavra sob o caret.
// Retorna o texto e (start,end) do match base. Se o documento estiver vazio
// ou não houver palavra, retorna QByteArray vazio.
QByteArray currentTokenForSearch(ScintillaEdit* editor, sptr_t& outStart, sptr_t& outEnd) {
    sptr_t selStart = editor->selectionStart();
    sptr_t selEnd   = editor->selectionEnd();

    if (selStart != selEnd) {
        outStart = selStart;
        outEnd   = selEnd;
        return editor->textRange(static_cast<int>(selStart), static_cast<int>(selEnd));
    }

    sptr_t pos = editor->currentPos();
    sptr_t ws  = editor->wordStartPosition(pos, true);
    sptr_t we  = editor->wordEndPosition(pos, true);
    if (ws == we) {
        return QByteArray();
    }
    outStart = ws;
    outEnd   = we;
    return editor->textRange(static_cast<int>(ws), static_cast<int>(we));
}

// Adiciona caret deslocado verticalmente (delta = +1 abaixo, -1 acima)
// preservando a coluna visual da seleção principal.
void addCaretLineDelta(ScintillaEdit* editor, int delta) {
    if (!editor) return;

    sptr_t mainSel = editor->mainSelection();
    sptr_t caret   = editor->selectionNCaret(mainSel);
    sptr_t line    = editor->lineFromPosition(caret);
    sptr_t col     = editor->column(caret);
    sptr_t totalLines = editor->send(SCI_GETLINECOUNT, 0, 0);

    sptr_t target = line + delta;
    if (target < 0 || target >= totalLines) {
        return;
    }

    sptr_t newPos = editor->findColumn(target, col);
    // findColumn pode retornar uma posição além do fim de linha; clampa.
    sptr_t lineEnd = editor->lineEndPosition(target);
    if (newPos < 0 || newPos > lineEnd) {
        newPos = lineEnd;
    }

    editor->addSelection(newPos, newPos);
}

} // namespace

MultiCursor::MultiCursor(QObject* parent)
    : QObject(parent) {
}

void MultiCursor::installFor(ScintillaEdit* editor) {
    if (!editor) return;

    applyMultiSelectionFlags(editor);

    // ScintillaEdit shadows QObject::property/setProperty with its own
    // (const char*, QByteArray)-flavored overloads, so qualify explicitly.
    if (editor->QObject::property(kInstalledProperty).toBool()) {
        return;
    }
    editor->QObject::setProperty(kInstalledProperty, true);

    QWidget* viewport = editor->viewport();
    if (viewport) {
        viewport->installEventFilter(sharedFilter());
    }
}

void MultiCursor::addCursorBelow(ScintillaEdit* editor) {
    addCaretLineDelta(editor, +1);
}

void MultiCursor::addCursorAbove(ScintillaEdit* editor) {
    addCaretLineDelta(editor, -1);
}

void MultiCursor::selectAllOccurrences(ScintillaEdit* editor) {
    if (!editor) return;

    sptr_t baseStart = 0;
    sptr_t baseEnd   = 0;
    QByteArray needle = currentTokenForSearch(editor, baseStart, baseEnd);
    if (needle.isEmpty()) {
        return;
    }

    // Match-case; whole word só quando o usuário não tinha seleção própria.
    bool selectionHadRange = (editor->selectionStart() != editor->selectionEnd());
    sptr_t flags = SCFIND_MATCHCASE;
    if (!selectionHadRange) {
        flags |= SCFIND_WHOLEWORD;
    }
    editor->setSearchFlags(flags);

    // Limpa seleções existentes e usa baseStart como âncora primária.
    editor->setSelection(baseEnd, baseStart);

    sptr_t docLen = editor->length();
    editor->setTargetRange(0, docLen);

    bool firstMatch = true;
    while (true) {
        sptr_t matchStart = editor->searchInTarget(needle.size(), needle.constData());
        if (matchStart < 0) {
            break;
        }
        sptr_t matchEnd = matchStart + needle.size();

        if (firstMatch) {
            editor->setSelection(matchEnd, matchStart);
            firstMatch = false;
        } else {
            editor->addSelection(matchEnd, matchStart);
        }

        // Avança o target para depois desta ocorrência.
        if (matchEnd == matchStart) {
            // Defesa contra match vazio (regex degenerado etc).
            ++matchEnd;
        }
        editor->setTargetRange(matchEnd, docLen);
    }
}

void MultiCursor::addNextOccurrence(ScintillaEdit* editor) {
    if (!editor) return;

    sptr_t baseStart = 0;
    sptr_t baseEnd   = 0;
    QByteArray needle = currentTokenForSearch(editor, baseStart, baseEnd);
    if (needle.isEmpty()) {
        return;
    }

    // Se não havia seleção (apenas palavra implícita), promove a palavra
    // a seleção principal e retorna — usuário invoca de novo para a próxima.
    bool hadExplicitSelection = (editor->selectionStart() != editor->selectionEnd());
    if (!hadExplicitSelection) {
        editor->setSelection(baseEnd, baseStart);
        return;
    }

    editor->setSearchFlags(SCFIND_MATCHCASE);

    // Procura a partir do fim da seleção principal atual.
    sptr_t mainSel  = editor->mainSelection();
    sptr_t mainEnd  = editor->selectionNEnd(mainSel);
    sptr_t docLen   = editor->length();

    auto findFrom = [&](sptr_t from, sptr_t to) -> sptr_t {
        editor->setTargetRange(from, to);
        return editor->searchInTarget(needle.size(), needle.constData());
    };

    sptr_t matchStart = findFrom(mainEnd, docLen);
    if (matchStart < 0) {
        // Wrap-around a partir do início.
        matchStart = findFrom(0, mainEnd);
        if (matchStart < 0) {
            return;
        }
    }
    sptr_t matchEnd = matchStart + needle.size();
    editor->addSelection(matchEnd, matchStart);
    editor->setMainSelection(editor->selections() - 1);
}

bool MultiCursor::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() != QEvent::MouseButtonPress) {
        return QObject::eventFilter(watched, event);
    }

    auto* mouse = static_cast<QMouseEvent*>(event);
    if (mouse->button() != Qt::LeftButton) {
        return QObject::eventFilter(watched, event);
    }

    // O watched é o viewport; o ScintillaEdit é o parent imediato (QAbstractScrollArea).
    QWidget* viewport = qobject_cast<QWidget*>(watched);
    if (!viewport) {
        return QObject::eventFilter(watched, event);
    }
    auto* editor = qobject_cast<ScintillaEdit*>(viewport->parent());
    if (!editor) {
        return QObject::eventFilter(watched, event);
    }

    Qt::KeyboardModifiers mods = mouse->modifiers();
    const bool ctrl = mods.testFlag(Qt::ControlModifier);
    const bool alt  = mods.testFlag(Qt::AltModifier);

    // Alt+drag → Scintilla cuida da seleção retangular sozinho graças ao
    // setMouseSelectionRectangularSwitch. Não interceptamos.
    if (alt && !ctrl) {
        return QObject::eventFilter(watched, event);
    }

    // Ctrl+Click esquerdo → adiciona caret na posição clicada sem mover o principal.
    if (ctrl && !alt) {
        const QPoint p = mouse->position().toPoint();
        sptr_t pos = editor->positionFromPoint(p.x(), p.y());
        if (pos >= 0) {
            editor->addSelection(pos, pos);
            return true; // consome o evento — Scintilla não move o caret principal
        }
    }

    return QObject::eventFilter(watched, event);
}
