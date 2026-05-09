#pragma once

#include <QObject>
#include <QPointer>
#include <QHash>
#include <QList>
#include <QString>

#include "LspClient.h"   // pulls LspLocation / LspSymbol / LspTextEdit / LspHover

class ScintillaEdit;
class EditorTab;
class MultiView;
class QShortcut;

// LspFeatures — bridges LspClient's request/response API to the active editor.
//
// Three things are wired:
//   - hover    : Scintilla's mouse-dwell event → textDocument/hover → QToolTip
//   - goto-def : F12 keybinding              → textDocument/definition → openFile + goto
//   - complete : Ctrl+Space keybinding       → textDocument/completion → autoCShow
//
// The class follows the same lifetime convention as the other M7 helpers
// (single instance owned by MainWindow). It needs to be told which editor is
// active; once attached it self-installs the dwell handler.
class LspFeatures : public QObject {
    Q_OBJECT
public:
    LspFeatures(LspClient* lsp, MultiView* multi, QObject* parent = nullptr);

    // Bind the helper to the editor of the active tab. Pass nullptr to detach.
    void setActiveTab(EditorTab* tab);

    // Public actions invoked by MainWindow shortcuts/menu items.
    void requestHoverAtCaret();
    void requestDefinitionAtCaret();
    void requestCompletionAtCaret();

    // M9 actions
    void requestSignatureHelpAtCaret();
    void requestReferencesAtCaret(bool includeDeclaration = true);
    void requestDocumentSymbolsCurrent();
    void requestWorkspaceSymbolsForQuery(const QString& query);
    void requestRenameAtCaret(const QString& newName);

    // M11 actions
    void requestCodeActionsAtCaret();
    void requestInlayHintsForVisibleRange();

    // M13
    void requestSemanticTokensCurrent();
    // Same call as requestDocumentSymbolsCurrent but fires outlineSymbolsReady
    // so MainWindow can route it to FunctionListPanel without competing with
    // the symbol picker dialog (which uses documentSymbolsReady).
    void requestOutlineSymbolsCurrent();

signals:
    // Emitted when the user activates a definition target — MainWindow handles
    // the actual file open + caret jump (it already has openFile + setActiveTab).
    void definitionResolved(const QString& filePath, int line, int column);

    // Emitted with the hover text so MainWindow can show it as a QToolTip.
    // x/y are global coordinates of the dwell point.
    void hoverReady(const QString& text, int globalX, int globalY);

    // M9 result signals — handled by MainWindow.
    void signatureHelpReady(const QString& text, int globalX, int globalY);
    void referencesReady(const QList<LspLocation>& locs);
    void documentSymbolsReady(const QList<LspSymbol>& syms);
    void workspaceSymbolsReady(const QList<LspSymbol>& syms);
    void renameEditsReady(const QList<LspTextEdit>& edits, const QString& newName);

    // M11
    void codeActionsReady(const QList<LspCodeAction>& actions);
    void inlayHintsReady(const QString& filePath, const QList<LspInlayHint>& hints);

    // M13
    void semanticTokensReady(const QString& filePath, const QList<LspSemanticToken>& tokens);
    void outlineSymbolsReady(const QString& filePath, const QList<LspSymbol>& syms);

private:
    // Per-editor wiring (one entry per editor we ever attached to).
    struct Bind {
        QPointer<ScintillaEdit> editor;
        bool dwellInstalled = false;
    };

    void ensureBoundEditor(ScintillaEdit* sci);
    void onDwellStart(int x, int y, int position);

    // LSP positions are line + UTF-16 character; Scintilla works in UTF-8 byte
    // offsets. These two helpers translate between the two for the *current*
    // active editor's text.
    bool sciPositionToLsp(ScintillaEdit* sci, int byteOffset,
                          int& outLine, int& outCharacter) const;
    int  lspPositionToSci(ScintillaEdit* sci, int line, int character) const;

    QPointer<LspClient>  m_lsp;
    QPointer<MultiView>  m_multi;
    QPointer<EditorTab>  m_activeTab;

    QHash<ScintillaEdit*, Bind> m_binds;
};
