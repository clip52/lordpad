#pragma once

#include <QObject>
#include <QPointer>

class QMenu;
class QAction;
class QWidget;
class ScintillaEdit;

// FoldingMenu
//
// Comandos de dobramento de codigo (code folding) ligados ao editor Scintilla.
// Fornece "Dobrar Tudo", "Desdobrar Tudo", "Dobrar/Desdobrar Linha Atual",
// e "Dobrar Ate o Nivel 1..9" via um submenu pre-construido em pt-BR.
//
// Tambem expoe enableFolding(editor) para configurar a margem de dobramento,
// markers, propriedades do lexer e folding automatico em um editor recem-criado.
class FoldingMenu : public QObject
{
    Q_OBJECT
public:
    explicit FoldingMenu(QObject* parent = nullptr);

    // Habilita o folding em um editor Scintilla recem-criado.
    // Chame uma unica vez por editor (e.g., em connectTabSignals).
    static void enableFolding(ScintillaEdit* editor);

    // Operacoes de dobramento.
    static void foldAll(ScintillaEdit* editor);
    static void unfoldAll(ScintillaEdit* editor);
    static void foldToLevel(ScintillaEdit* editor, int level);   // level em [1..9]
    static void toggleCurrentFold(ScintillaEdit* editor);

    // Cria um submenu "Dobramento" pronto para encaixar em outro menu (e.g. Exibir).
    // O parent e o dono Qt do submenu (geralmente a MainWindow).
    QMenu* createMenu(QWidget* parent);

    // Ativa/desativa as acoes do menu de acordo com a presenca de um editor.
    void setActiveEditor(ScintillaEdit* editor);

private:
    void updateActionsEnabled();

    QPointer<ScintillaEdit> m_editor;

    QPointer<QMenu>   m_menu;
    QPointer<QMenu>   m_levelMenu;

    QPointer<QAction> m_foldAllAction;
    QPointer<QAction> m_unfoldAllAction;
    QPointer<QAction> m_toggleAction;
    QPointer<QAction> m_levelActions[9];
};
