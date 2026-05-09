#include "FoldingMenu.h"

#include <QAction>
#include <QMenu>
#include <QWidget>

#include "ScintillaEdit.h"

namespace {

// Margem reservada para o folding (margem 2 por convencao do Notepad++).
constexpr int kFoldMarginIndex = 2;
constexpr int kFoldMarginWidth = 14;

// Cor "neutra" (sera sobreposta pelo tema; aqui passamos o sptr_t direto).
constexpr long kFoldFore = 0x808080; // cinza
constexpr long kFoldBack = 0xFFFFFF; // branco

// Mapa (markerNumber -> shape) para os markers de fold.
struct FoldMarker {
    int markerNumber;
    int shape;
};

constexpr FoldMarker kFoldMarkers[] = {
    { SC_MARKNUM_FOLDEROPEN,    SC_MARK_BOXMINUS          },
    { SC_MARKNUM_FOLDER,        SC_MARK_BOXPLUS           },
    { SC_MARKNUM_FOLDERSUB,     SC_MARK_VLINE             },
    { SC_MARKNUM_FOLDERTAIL,    SC_MARK_LCORNER           },
    { SC_MARKNUM_FOLDEREND,     SC_MARK_BOXPLUSCONNECTED  },
    { SC_MARKNUM_FOLDEROPENMID, SC_MARK_BOXMINUSCONNECTED },
    { SC_MARKNUM_FOLDERMIDTAIL, SC_MARK_TCORNER           },
};

// Helper: envia SCI_SETPROPERTY com strings em wParam/lParam.
void setSciProperty(ScintillaEdit* editor, const char* key, const char* value)
{
    editor->send(SCI_SETPROPERTY,
                 reinterpret_cast<uptr_t>(key),
                 reinterpret_cast<sptr_t>(value));
}

} // namespace

FoldingMenu::FoldingMenu(QObject* parent)
    : QObject(parent)
{
}

void FoldingMenu::enableFolding(ScintillaEdit* editor)
{
    if (!editor)
        return;

    // 1) Propriedades do lexer que ativam o folding.
    setSciProperty(editor, "fold",         "1");
    setSciProperty(editor, "fold.compact", "0");
    // Heuristicas comuns adicionais — inofensivas em lexers que as ignoram.
    setSciProperty(editor, "fold.comment",          "1");
    setSciProperty(editor, "fold.preprocessor",     "1");
    setSciProperty(editor, "fold.html",             "1");
    setSciProperty(editor, "fold.html.preprocessor","1");

    // 2) Configura a margem 2 como margem de simbolo dedicada ao folding.
    editor->send(SCI_SETMARGINTYPEN,      kFoldMarginIndex, SC_MARGIN_SYMBOL);
    editor->send(SCI_SETMARGINMASKN,      kFoldMarginIndex, SC_MASK_FOLDERS);
    editor->send(SCI_SETMARGINSENSITIVEN, kFoldMarginIndex, 1);
    editor->send(SCI_SETMARGINWIDTHN,     kFoldMarginIndex, kFoldMarginWidth);

    // 3) Define os shapes e cores para os markers de fold.
    for (const FoldMarker& fm : kFoldMarkers) {
        editor->send(SCI_MARKERDEFINE,  fm.markerNumber, fm.shape);
        editor->send(SCI_MARKERSETFORE, fm.markerNumber, kFoldBack);
        editor->send(SCI_MARKERSETBACK, fm.markerNumber, kFoldFore);
    }

    // 4) Folding automatico: desenhar/esconder linhas, click reage, atualizacao.
    editor->send(SCI_SETAUTOMATICFOLD,
                 SC_AUTOMATICFOLD_SHOW
                 | SC_AUTOMATICFOLD_CLICK
                 | SC_AUTOMATICFOLD_CHANGE);
}

void FoldingMenu::foldAll(ScintillaEdit* editor)
{
    if (!editor)
        return;

    const sptr_t lineCount = editor->send(SCI_GETLINECOUNT);
    for (sptr_t line = 0; line < lineCount; ++line) {
        const sptr_t level = editor->send(SCI_GETFOLDLEVEL, line);
        if (level & SC_FOLDLEVELHEADERFLAG) {
            editor->send(SCI_FOLDLINE, line, SC_FOLDACTION_CONTRACT);
        }
    }
}

void FoldingMenu::unfoldAll(ScintillaEdit* editor)
{
    if (!editor)
        return;

    const sptr_t lineCount = editor->send(SCI_GETLINECOUNT);
    for (sptr_t line = 0; line < lineCount; ++line) {
        const sptr_t level = editor->send(SCI_GETFOLDLEVEL, line);
        if (level & SC_FOLDLEVELHEADERFLAG) {
            editor->send(SCI_FOLDLINE, line, SC_FOLDACTION_EXPAND);
        }
    }
}

void FoldingMenu::foldToLevel(ScintillaEdit* editor, int level)
{
    if (!editor)
        return;
    if (level < 1 || level > 9)
        return;

    // Scintilla representa o nivel base como SC_FOLDLEVELBASE (0x400).
    // Para "Dobrar ate o nivel N", contraimos cabecalhos cujo nivel == BASE+N
    // e expandimos cabecalhos com nivel < BASE+N (niveis exteriores).
    const sptr_t targetLevel = static_cast<sptr_t>(0x400 + level);
    const sptr_t lineCount   = editor->send(SCI_GETLINECOUNT);

    for (sptr_t line = 0; line < lineCount; ++line) {
        const sptr_t raw = editor->send(SCI_GETFOLDLEVEL, line);
        if (!(raw & SC_FOLDLEVELHEADERFLAG))
            continue;

        const sptr_t lvl = raw & SC_FOLDLEVELNUMBERMASK;   // 0xFFF
        if (lvl == targetLevel) {
            editor->send(SCI_FOLDLINE, line, SC_FOLDACTION_CONTRACT);
        } else if (lvl < targetLevel) {
            editor->send(SCI_FOLDLINE, line, SC_FOLDACTION_EXPAND);
        }
        // lvl > targetLevel: linha pertence a um bloco interno, ja sera
        // ocultada pelo contract do header pai; nao mexemos aqui.
    }
}

void FoldingMenu::toggleCurrentFold(ScintillaEdit* editor)
{
    if (!editor)
        return;

    const sptr_t pos  = editor->send(SCI_GETCURRENTPOS);
    const sptr_t line = editor->send(SCI_LINEFROMPOSITION, pos);

    // Se a linha atual nao e um header, sobe ate achar o header pai.
    sptr_t target = line;
    while (target >= 0) {
        const sptr_t lvl = editor->send(SCI_GETFOLDLEVEL, target);
        if (lvl & SC_FOLDLEVELHEADERFLAG)
            break;
        --target;
    }
    if (target < 0)
        target = line;

    editor->send(SCI_FOLDLINE, target, SC_FOLDACTION_TOGGLE);
}

QMenu* FoldingMenu::createMenu(QWidget* parent)
{
    if (m_menu)
        return m_menu;

    m_menu = new QMenu(tr("&Dobramento"), parent);

    m_foldAllAction = m_menu->addAction(tr("Dobrar &Tudo"));
    connect(m_foldAllAction, &QAction::triggered, this, [this]() {
        if (m_editor) foldAll(m_editor);
    });

    m_unfoldAllAction = m_menu->addAction(tr("&Desdobrar Tudo"));
    connect(m_unfoldAllAction, &QAction::triggered, this, [this]() {
        if (m_editor) unfoldAll(m_editor);
    });

    m_menu->addSeparator();

    m_toggleAction = m_menu->addAction(tr("&Alternar Dobramento da Linha Atual"));
    connect(m_toggleAction, &QAction::triggered, this, [this]() {
        if (m_editor) toggleCurrentFold(m_editor);
    });

    m_menu->addSeparator();

    m_levelMenu = m_menu->addMenu(tr("Dobrar ate o &Nivel"));
    for (int i = 0; i < 9; ++i) {
        const int level = i + 1;
        QAction* act = m_levelMenu->addAction(tr("Nivel %1").arg(level));
        m_levelActions[i] = act;
        connect(act, &QAction::triggered, this, [this, level]() {
            if (m_editor) foldToLevel(m_editor, level);
        });
    }

    updateActionsEnabled();
    return m_menu;
}

void FoldingMenu::setActiveEditor(ScintillaEdit* editor)
{
    m_editor = editor;
    updateActionsEnabled();
}

void FoldingMenu::updateActionsEnabled()
{
    const bool enabled = !m_editor.isNull();

    if (m_foldAllAction)   m_foldAllAction->setEnabled(enabled);
    if (m_unfoldAllAction) m_unfoldAllAction->setEnabled(enabled);
    if (m_toggleAction)    m_toggleAction->setEnabled(enabled);
    if (m_levelMenu)       m_levelMenu->setEnabled(enabled);
    for (QAction* a : m_levelActions) {
        if (a) a->setEnabled(enabled);
    }
}
