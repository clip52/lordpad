#pragma once

#include <QObject>
#include <QHash>
#include <QList>
#include <QMetaType>
#include <QString>
#include <QByteArray>
#include <QStringList>
#include <QPointer>

#include <functional>

class QProcess;
class QTimer;
class QJsonValue;
class QJsonObject;

// One diagnostic entry produced by a language server (LSP publishDiagnostics).
struct LspDiagnostic {
    QString message;
    QString severity;   // "error" | "warning" | "info" | "hint"
    int     line   = 0; // 0-based, matches LSP convention
    int     column = 0; // 0-based, matches LSP convention
    QString source;     // "clangd", "pyright", "gopls", ...
    QString code;       // optional diagnostic code (may be empty)
};

// One hover result. Plain-text rendering of the LSP hover contents.
struct LspHover {
    QString text;   // empty when the server returned no hover info
};

// One target location resolved from textDocument/definition.
struct LspLocation {
    QString filePath;   // absolute path on disk
    int     line   = 0; // 0-based
    int     column = 0; // 0-based
};

// One completion item. Mirrors a small subset of the LSP CompletionItem.
struct LspCompletionItem {
    QString label;        // shown in the popup
    QString insertText;   // what to actually insert (falls back to label when empty)
    QString detail;       // type signature / module / etc.
    int     kind = 0;     // LSP CompletionItemKind (1..25), 0 = unknown
};

// Subset of LSP SignatureHelp — one signature with raw text and the index of
// the currently active signature/parameter so the UI can highlight it.
struct LspSignatureHelp {
    QStringList signatures;        // pre-formatted "label" of each overload
    int         activeSignature = 0;
    int         activeParameter = 0;
};

// Used for both find-references results and rename text edits below.
struct LspTextEdit {
    QString filePath;
    int     startLine = 0, startColumn = 0;
    int     endLine   = 0, endColumn   = 0;
    QString newText;               // empty when the result is just a "reference" pointer
};

// One actionable item from textDocument/codeAction. We only carry the parts
// we know how to execute on the client side: a title and a list of edits to
// apply. Code actions that arrive as `command` only (no inline edit and no
// `data` for resolve) are surfaced with `edits` empty — the UI shows them
// disabled with a helpful tooltip.
struct LspCodeAction {
    QString title;
    QString kind;                  // "quickfix", "refactor", ...
    QList<LspTextEdit> edits;      // empty when the action requires `codeAction/resolve`
    bool    isCommandOnly = false;
};

// One semantic token (M13). tokenType is the resolved name from the server's
// legend ("function", "variable", "parameter", "type", ...). When the server
// didn't expose a legend, tokenType falls back to the raw integer as text.
struct LspSemanticToken {
    int     line   = 0;
    int     column = 0;
    int     length = 0;
    QString tokenType;
};

// One inlay hint — a label rendered between characters (parameter names,
// inferred types, return type annotations).
struct LspInlayHint {
    int     line   = 0;
    int     column = 0;
    QString label;
    int     kind   = 0;            // 1=Type, 2=Parameter, 0=unknown
    bool    paddingLeft  = false;
    bool    paddingRight = false;
};

// One row in the document-symbol / workspace-symbol response.
struct LspSymbol {
    QString name;
    QString containerName;         // class name for methods, etc., may be empty
    int     kind = 0;              // LSP SymbolKind (1..26)
    QString filePath;              // workspace-symbol fills this; document-symbol uses the active path
    int     line   = 0;
    int     column = 0;
};

Q_DECLARE_METATYPE(LspLocation)
Q_DECLARE_METATYPE(LspSymbol)
Q_DECLARE_METATYPE(LspCodeAction)

// Minimal Language Server Protocol client.
//
// This is a BASIC implementation that handles only the diagnostics flow:
//   - lazy-launches one server per language via QProcess
//   - JSON-RPC 2.0 framing (Content-Length: N\r\n\r\n<body>)
//   - sends initialize / initialized / didOpen / didChange / didClose
//   - parses textDocument/publishDiagnostics and emits diagnosticsUpdated
//
// Completion, hover, goto-definition, code actions, etc. are intentionally
// NOT implemented here — this is the foundation for a later phase.
class LspClient : public QObject {
    Q_OBJECT
public:
    explicit LspClient(QObject* parent = nullptr);
    ~LspClient() override;

    // True if the given lexer name maps to a known language server (and the
    // command resolves on PATH or via QSettings override).
    bool isLanguageSupported(const QString& lexerName) const;

    // Notify a file is opened. Launches the matching server lazily on first
    // call for that language. Subsequent didOpen for additional files of the
    // same language reuses the running server.
    void didOpen(const QString& filePath, const QString& lexerName, const QString& content);

    // Notify a file changed. Coalesced (debounced 500 ms) per-file before
    // hitting the wire — caller can fire on every keystroke.
    void didChange(const QString& filePath, const QString& content);

    // Notify a file is closed.
    void didClose(const QString& filePath);

    // Send shutdown + exit to every running server. Call from MainWindow::closeEvent.
    void shutdownAll();

    // Accessor for the diagnostic cache (keyed by absolute file path).
    QList<LspDiagnostic> diagnosticsFor(const QString& filePath) const;

    // ---- M8: on-demand requests --------------------------------------------
    // Each of these is a no-op when the server isn't ready yet; the callback
    // is fired exactly once when the server replies (or is dropped on shutdown).
    // line / column are 0-based, matching LSP's textDocument/* convention.

    using HoverCallback         = std::function<void(const LspHover&)>;
    using DefinitionCallback    = std::function<void(const QList<LspLocation>&)>;
    using CompletionCallback    = std::function<void(const QList<LspCompletionItem>&)>;
    using SignatureCallback     = std::function<void(const LspSignatureHelp&)>;
    using ReferencesCallback    = std::function<void(const QList<LspLocation>&)>;
    using DocumentSymbolCallback  = std::function<void(const QList<LspSymbol>&)>;
    using WorkspaceSymbolCallback = std::function<void(const QList<LspSymbol>&)>;
    using RenameCallback        = std::function<void(const QList<LspTextEdit>&)>;
    using CodeActionCallback    = std::function<void(const QList<LspCodeAction>&)>;
    using InlayHintCallback     = std::function<void(const QList<LspInlayHint>&)>;
    using SemanticTokensCallback = std::function<void(const QList<LspSemanticToken>&)>;

    void requestHover(const QString& filePath, int line, int column,
                      HoverCallback cb);
    void requestDefinition(const QString& filePath, int line, int column,
                           DefinitionCallback cb);
    void requestCompletion(const QString& filePath, int line, int column,
                           CompletionCallback cb);

    // M9: signature help — invoked at cursor (typically inside parens).
    void requestSignatureHelp(const QString& filePath, int line, int column,
                              SignatureCallback cb);

    // M9: find references at cursor. includeDeclaration=true also reports
    // the symbol's defining occurrence.
    void requestReferences(const QString& filePath, int line, int column,
                           bool includeDeclaration, ReferencesCallback cb);

    // M9: outline of the active document.
    void requestDocumentSymbols(const QString& filePath, DocumentSymbolCallback cb);

    // M9: cross-file fuzzy symbol search. `query` is matched against the
    // server's index — typically by substring or fuzzy.
    void requestWorkspaceSymbols(const QString& query, WorkspaceSymbolCallback cb);

    // M9: rename a symbol. The result is a flat list of edits to apply
    // verbatim across (possibly multiple) files. An empty list means the
    // server refused or the operation is unsupported.
    void requestRename(const QString& filePath, int line, int column,
                       const QString& newName, RenameCallback cb);

    // M11: code actions for the given line (range = line start → line end).
    // Diagnostics already received for that file are passed along so the
    // server can pick quickfixes that apply.
    void requestCodeActions(const QString& filePath, int line,
                            CodeActionCallback cb);

    // M11: inlay hints for a range of lines. Servers that don't support
    // inlay hints reply with null/[] — callback gets an empty list.
    void requestInlayHints(const QString& filePath, int startLine, int endLine,
                           InlayHintCallback cb);

    // M13: full-document semantic tokens. The result is decoded from the
    // delta-encoded LSP wire format into a flat list with line/column/length
    // and a resolved tokenType string (using the server's initialize legend).
    void requestSemanticTokens(const QString& filePath, SemanticTokensCallback cb);

signals:
    void diagnosticsUpdated(const QString& filePath, const QList<LspDiagnostic>& diags);
    void serverError(const QString& serverName, const QString& message);

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, int exitStatus);
    void onProcessErrorOccurred(int error);
    void flushPendingChange();

private:
    // Generic JSON-RPC response handler. result is JSON-parsed; isError tells
    // the caller whether to look at result as an error object or success body.
    using ResponseCallback = std::function<void(const QJsonValue& result, bool isError)>;

    // One running server instance plus its parser state and bookkeeping.
    struct ServerEntry {
        QString    languageId;       // canonical LSP languageId ("cpp", "python", ...)
        QString    command;          // resolved executable (e.g. "clangd")
        QStringList args;            // CLI args
        QString    serverName;       // human-readable name for signals ("clangd", "pyright", ...)
        QString    rootUri;          // file:// URI of the project root we initialized with
        QPointer<QProcess> proc;     // the running language server
        QByteArray buffer;           // incoming-byte ring (split header/body)
        bool       initialized = false; // becomes true after we receive the initialize response
        int        nextRequestId = 1;
        QList<QByteArray> queuedAfterInit;        // messages held until initialized response arrives
        QHash<QString, int> openVersions;         // file URI → last sent version (for didChange)
        QHash<int, ResponseCallback> pending;     // request id → callback awaiting a response
        QStringList semanticTokenTypes;           // M13: legend captured from initialize response
    };

    // Look up (or build) a server entry for a given LSP language id.
    // Returns nullptr if no command is configured / available.
    ServerEntry* ensureServerForLanguage(const QString& languageId);

    // Map an internal lexer name (cpp, python, ...) to LSP languageId.
    static QString lexerToLanguageId(const QString& lexerName);

    // Default executable + args per languageId. Reads QSettings overrides.
    void resolveCommandForLanguage(const QString& languageId,
                                   QString& outCmd,
                                   QStringList& outArgs,
                                   QString& outFriendlyName) const;

    // JSON-RPC plumbing.
    void sendMessage(ServerEntry* srv, const QByteArray& jsonBody);
    // sendRequest registers `cb` against the new request id (when non-null) so
    // handleIncomingMessage can dispatch the response. Returns the assigned id
    // (>=1 on success, 0 if the request couldn't be sent).
    int  sendRequest(ServerEntry* srv, const QString& method,
                     const QByteArray& paramsJson, ResponseCallback cb = {});
    void sendNotification(ServerEntry* srv, const QString& method, const QByteArray& paramsJson);
    void processIncoming(ServerEntry* srv);
    void handleIncomingMessage(ServerEntry* srv, const QByteArray& body);

    // Initialize handshake.
    void sendInitialize(ServerEntry* srv);
    void sendInitialized(ServerEntry* srv);

    // Helpers.
    static QString fileUri(const QString& filePath);
    static QString filePathFromUri(const QString& uri);
    static QByteArray jsonEscape(const QString& s);
    static QString rootDirForFile(const QString& filePath);

    // M11: parse a WorkspaceEdit JSON shape into a flat edit list.
    // Used by both rename and code actions.
    static QList<LspTextEdit> parseWorkspaceEdit(const QJsonObject& edit);

    // Per-file state independent of language.
    struct FileState {
        QString languageId;
        QString lexerName;
        QString lastContent;
        int     version = 0;
        bool    isOpen  = false;
    };

    QHash<QString, ServerEntry*> m_serversByLang; // languageId → ServerEntry
    QHash<QString, FileState>    m_files;         // absolute file path → state
    QHash<QString, QList<LspDiagnostic>> m_diagnostics; // absolute file path → diags

    // didChange debounce: per-file 500 ms timer. We share a single QTimer
    // and a queue of pending file paths to flush.
    QTimer* m_changeTimer = nullptr;
    QHash<QString, QString> m_pendingContent; // filePath → newest content
};
