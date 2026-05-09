#include "LspClient.h"

#include <QProcess>
#include <QTimer>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QSettings>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QCoreApplication>
#include <QDebug>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

QString LspClient::fileUri(const QString& filePath)
{
    return QUrl::fromLocalFile(filePath).toString();
}

QString LspClient::filePathFromUri(const QString& uri)
{
    return QUrl(uri).toLocalFile();
}

QByteArray LspClient::jsonEscape(const QString& s)
{
    // Cheap escape: round-trip through QJsonValue.
    QJsonDocument d(QJsonObject{{"v", s}});
    QByteArray out = d.toJson(QJsonDocument::Compact);
    // Strip {"v":  ...  } wrapper. We just want the escaped string literal.
    int a = out.indexOf("\"v\":");
    if (a < 0) return QByteArray("\"\"");
    a += 4;
    int b = out.lastIndexOf('}');
    if (b < 0) b = out.size();
    return out.mid(a, b - a).trimmed();
}

QString LspClient::rootDirForFile(const QString& filePath)
{
    QFileInfo fi(filePath);
    QDir dir = fi.absoluteDir();

    // Try to walk up to a project marker. Falls back to the file's directory.
    static const QStringList markers = {
        ".git", "compile_commands.json", "CMakeLists.txt",
        "pyproject.toml", "setup.py", "go.mod", "Cargo.toml",
        "package.json", "tsconfig.json"
    };
    for (int depth = 0; depth < 8; ++depth) {
        for (const QString& m : markers) {
            if (dir.exists(m)) return dir.absolutePath();
        }
        if (!dir.cdUp()) break;
    }
    return fi.absolutePath();
}

QString LspClient::lexerToLanguageId(const QString& lexerName)
{
    const QString l = lexerName.toLower();
    if (l == "cpp" || l == "c++") return "cpp";
    if (l == "c")                 return "c";
    if (l == "python")            return "python";
    if (l == "go")                return "go";
    if (l == "rust")              return "rust";
    if (l == "typescript")        return "typescript";
    if (l == "javascript")        return "javascript";
    return QString();
}

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

LspClient::LspClient(QObject* parent)
    : QObject(parent)
{
    m_changeTimer = new QTimer(this);
    m_changeTimer->setSingleShot(true);
    m_changeTimer->setInterval(500);
    connect(m_changeTimer, &QTimer::timeout, this, &LspClient::flushPendingChange);
}

LspClient::~LspClient()
{
    shutdownAll();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool LspClient::isLanguageSupported(const QString& lexerName) const
{
    const QString lang = lexerToLanguageId(lexerName);
    if (lang.isEmpty()) return false;

    QString cmd, friendly;
    QStringList args;
    const_cast<LspClient*>(this)->resolveCommandForLanguage(lang, cmd, args, friendly);
    if (cmd.isEmpty()) return false;

    // Allow if it's an absolute path that exists, or if it resolves on PATH.
    if (QFileInfo::exists(cmd)) return true;
    return !QStandardPaths::findExecutable(cmd).isEmpty();
}

void LspClient::didOpen(const QString& filePath, const QString& lexerName, const QString& content)
{
    if (filePath.isEmpty()) return;

    const QString languageId = lexerToLanguageId(lexerName);
    if (languageId.isEmpty()) return; // unknown language — nothing to do

    ServerEntry* srv = ensureServerForLanguage(languageId);
    if (!srv) return; // server not installed / disabled

    FileState st;
    st.languageId  = languageId;
    st.lexerName   = lexerName;
    st.lastContent = content;
    st.version     = 1;
    st.isOpen      = true;
    m_files.insert(filePath, st);

    srv->openVersions.insert(fileUri(filePath), 1);

    // Build didOpen params:
    // { textDocument: { uri, languageId, version, text } }
    QByteArray params;
    params  = "{\"textDocument\":{\"uri\":";
    params += jsonEscape(fileUri(filePath));
    params += ",\"languageId\":";
    params += jsonEscape(languageId);
    params += ",\"version\":1,\"text\":";
    params += jsonEscape(content);
    params += "}}";

    sendNotification(srv, "textDocument/didOpen", params);
}

void LspClient::didChange(const QString& filePath, const QString& content)
{
    if (!m_files.contains(filePath)) return;
    m_pendingContent.insert(filePath, content);
    if (!m_changeTimer->isActive()) m_changeTimer->start();
}

void LspClient::flushPendingChange()
{
    // Drain everything that accumulated during the debounce window.
    const auto pendingCopy = m_pendingContent;
    m_pendingContent.clear();

    for (auto it = pendingCopy.begin(); it != pendingCopy.end(); ++it) {
        const QString& filePath = it.key();
        const QString& content  = it.value();
        auto fit = m_files.find(filePath);
        if (fit == m_files.end()) continue;

        ServerEntry* srv = m_serversByLang.value(fit->languageId, nullptr);
        if (!srv) continue;

        fit->version    += 1;
        fit->lastContent = content;
        srv->openVersions.insert(fileUri(filePath), fit->version);

        // Full-document sync (TextDocumentSyncKind.Full).
        QByteArray params;
        params  = "{\"textDocument\":{\"uri\":";
        params += jsonEscape(fileUri(filePath));
        params += ",\"version\":";
        params += QByteArray::number(fit->version);
        params += "},\"contentChanges\":[{\"text\":";
        params += jsonEscape(content);
        params += "}]}";

        sendNotification(srv, "textDocument/didChange", params);
    }
}

void LspClient::didClose(const QString& filePath)
{
    auto fit = m_files.find(filePath);
    if (fit == m_files.end()) return;

    ServerEntry* srv = m_serversByLang.value(fit->languageId, nullptr);
    if (srv) {
        QByteArray params;
        params  = "{\"textDocument\":{\"uri\":";
        params += jsonEscape(fileUri(filePath));
        params += "}}";
        sendNotification(srv, "textDocument/didClose", params);
        srv->openVersions.remove(fileUri(filePath));
    }

    m_files.erase(fit);
    m_pendingContent.remove(filePath);
    m_diagnostics.remove(filePath);
}

void LspClient::shutdownAll()
{
    // Steal the map first so the QProcess::finished slot won't double-free
    // entries while we wait for each server to exit.
    QHash<QString, ServerEntry*> toShutdown;
    toShutdown.swap(m_serversByLang);

    for (auto it = toShutdown.begin(); it != toShutdown.end(); ++it) {
        ServerEntry* srv = it.value();
        if (!srv) continue;

        // Disconnect lifecycle signals so onProcessFinished/onProcessErrorOccurred
        // won't try to look the entry up in the (now empty) map and delete it.
        if (srv->proc) {
            disconnect(srv->proc.data(), nullptr, this, nullptr);
        }

        if (srv->proc && srv->proc->state() == QProcess::Running) {
            // Best-effort orderly shutdown.
            sendRequest(srv, "shutdown", "null");
            sendNotification(srv, "exit", "null");
            srv->proc->waitForFinished(800);
            if (srv->proc->state() == QProcess::Running) {
                srv->proc->terminate();
                srv->proc->waitForFinished(400);
            }
            if (srv->proc->state() == QProcess::Running) {
                srv->proc->kill();
                srv->proc->waitForFinished(200);
            }
        }
        if (srv->proc) srv->proc->deleteLater();
        delete srv;
    }
}

QList<LspDiagnostic> LspClient::diagnosticsFor(const QString& filePath) const
{
    return m_diagnostics.value(filePath);
}

// ---------------------------------------------------------------------------
// M8: on-demand requests (hover / definition / completion)
// ---------------------------------------------------------------------------
namespace {

// Build the textDocument/position params block the three on-demand requests share.
//   { textDocument: { uri }, position: { line, character } }
QByteArray buildPositionParams(const QString& uri, int line, int column,
                               QByteArray (*esc)(const QString&))
{
    QByteArray p;
    p  = "{\"textDocument\":{\"uri\":";
    p += esc(uri);
    p += "},\"position\":{\"line\":";
    p += QByteArray::number(line);
    p += ",\"character\":";
    p += QByteArray::number(column);
    p += "}}";
    return p;
}

// LSP "hover.contents" can be:
//   - a MarkupContent { kind, value }
//   - a MarkedString (string or { language, value })
//   - an array of either
// Render down to a single plain-text string.
QString flattenHoverContents(const QJsonValue& contents)
{
    if (contents.isString()) {
        return contents.toString();
    }
    if (contents.isObject()) {
        const QJsonObject o = contents.toObject();
        if (o.contains("value")) return o.value("value").toString();
        return QString();
    }
    if (contents.isArray()) {
        QStringList parts;
        for (const QJsonValue& v : contents.toArray()) {
            const QString s = flattenHoverContents(v);
            if (!s.isEmpty()) parts.append(s);
        }
        return parts.join(QStringLiteral("\n\n"));
    }
    return QString();
}

} // namespace

void LspClient::requestHover(const QString& filePath, int line, int column,
                             HoverCallback cb)
{
    if (!cb) return;
    auto fit = m_files.find(filePath);
    if (fit == m_files.end()) { cb(LspHover{}); return; }

    ServerEntry* srv = m_serversByLang.value(fit->languageId, nullptr);
    if (!srv) { cb(LspHover{}); return; }

    const QByteArray params = buildPositionParams(fileUri(filePath), line, column, &jsonEscape);
    sendRequest(srv, "textDocument/hover", params,
        [cb](const QJsonValue& result, bool isError) {
            LspHover out;
            if (!isError && result.isObject()) {
                out.text = flattenHoverContents(result.toObject().value("contents"));
            }
            cb(out);
        });
}

void LspClient::requestDefinition(const QString& filePath, int line, int column,
                                  DefinitionCallback cb)
{
    if (!cb) return;
    auto fit = m_files.find(filePath);
    if (fit == m_files.end()) { cb({}); return; }

    ServerEntry* srv = m_serversByLang.value(fit->languageId, nullptr);
    if (!srv) { cb({}); return; }

    const QByteArray params = buildPositionParams(fileUri(filePath), line, column, &jsonEscape);
    sendRequest(srv, "textDocument/definition", params,
        [cb](const QJsonValue& result, bool isError) {
            QList<LspLocation> locs;
            if (isError) { cb(locs); return; }

            // result may be: Location, Location[], LocationLink[], or null.
            auto pushLocation = [&](const QJsonObject& o) {
                LspLocation l;
                // LocationLink uses targetUri/targetSelectionRange; Location uses uri/range.
                const QString uri = o.value("uri").toString(o.value("targetUri").toString());
                if (uri.isEmpty()) return;
                l.filePath = filePathFromUri(uri);
                const QJsonObject range = o.contains("range")
                                              ? o.value("range").toObject()
                                              : o.value("targetSelectionRange").toObject();
                const QJsonObject start = range.value("start").toObject();
                l.line   = start.value("line").toInt();
                l.column = start.value("character").toInt();
                locs.push_back(l);
            };

            if (result.isObject()) {
                pushLocation(result.toObject());
            } else if (result.isArray()) {
                for (const QJsonValue& v : result.toArray()) {
                    if (v.isObject()) pushLocation(v.toObject());
                }
            }
            cb(locs);
        });
}

void LspClient::requestCompletion(const QString& filePath, int line, int column,
                                  CompletionCallback cb)
{
    if (!cb) return;
    auto fit = m_files.find(filePath);
    if (fit == m_files.end()) { cb({}); return; }

    ServerEntry* srv = m_serversByLang.value(fit->languageId, nullptr);
    if (!srv) { cb({}); return; }

    QByteArray params = buildPositionParams(fileUri(filePath), line, column, &jsonEscape);
    // Inject `context: { triggerKind: 1 (Invoked) }` so servers that gate
    // results on context (e.g. clangd) don't bail out with an empty list.
    if (params.endsWith('}')) {
        params.chop(1);
        params += ",\"context\":{\"triggerKind\":1}}";
    }

    sendRequest(srv, "textDocument/completion", params,
        [cb](const QJsonValue& result, bool isError) {
            QList<LspCompletionItem> items;
            if (isError) { cb(items); return; }

            QJsonArray arr;
            if (result.isArray()) {
                arr = result.toArray();
            } else if (result.isObject()) {
                arr = result.toObject().value("items").toArray();
            }
            items.reserve(arr.size());
            for (const QJsonValue& v : arr) {
                const QJsonObject o = v.toObject();
                LspCompletionItem ci;
                ci.label      = o.value("label").toString();
                ci.insertText = o.value("insertText").toString(ci.label);
                ci.detail     = o.value("detail").toString();
                ci.kind       = o.value("kind").toInt(0);
                if (!ci.label.isEmpty()) items.push_back(ci);
            }
            cb(items);
        });
}

// ---------------------------------------------------------------------------
// M9: signature help / references / document & workspace symbols / rename
// ---------------------------------------------------------------------------
namespace {

// Helper: range obj → 4 ints into an LspTextEdit.
void rangeIntoEdit(const QJsonObject& range, LspTextEdit& edit) {
    const QJsonObject s = range.value("start").toObject();
    const QJsonObject e = range.value("end").toObject();
    edit.startLine   = s.value("line").toInt();
    edit.startColumn = s.value("character").toInt();
    edit.endLine     = e.value("line").toInt();
    edit.endColumn   = e.value("character").toInt();
}

} // namespace

void LspClient::requestSignatureHelp(const QString& filePath, int line, int column,
                                     SignatureCallback cb)
{
    if (!cb) return;
    auto fit = m_files.find(filePath);
    if (fit == m_files.end()) { cb({}); return; }
    ServerEntry* srv = m_serversByLang.value(fit->languageId, nullptr);
    if (!srv) { cb({}); return; }

    const QByteArray params = buildPositionParams(fileUri(filePath), line, column, &jsonEscape);
    sendRequest(srv, "textDocument/signatureHelp", params,
        [cb](const QJsonValue& result, bool isError) {
            LspSignatureHelp out;
            if (isError || !result.isObject()) { cb(out); return; }
            const QJsonObject o = result.toObject();
            out.activeSignature = o.value("activeSignature").toInt(0);
            out.activeParameter = o.value("activeParameter").toInt(0);
            for (const QJsonValue& s : o.value("signatures").toArray()) {
                const QJsonObject so = s.toObject();
                QString label = so.value("label").toString();
                if (!label.isEmpty()) out.signatures.append(label);
            }
            cb(out);
        });
}

void LspClient::requestReferences(const QString& filePath, int line, int column,
                                  bool includeDeclaration, ReferencesCallback cb)
{
    if (!cb) return;
    auto fit = m_files.find(filePath);
    if (fit == m_files.end()) { cb({}); return; }
    ServerEntry* srv = m_serversByLang.value(fit->languageId, nullptr);
    if (!srv) { cb({}); return; }

    QByteArray params = buildPositionParams(fileUri(filePath), line, column, &jsonEscape);
    // Append the `context` object before the closing brace.
    if (params.endsWith('}')) {
        params.chop(1);
        params += ",\"context\":{\"includeDeclaration\":";
        params += (includeDeclaration ? "true" : "false");
        params += "}}";
    }
    sendRequest(srv, "textDocument/references", params,
        [cb](const QJsonValue& result, bool isError) {
            QList<LspLocation> out;
            if (isError || !result.isArray()) { cb(out); return; }
            for (const QJsonValue& v : result.toArray()) {
                const QJsonObject o = v.toObject();
                const QString uri = o.value("uri").toString();
                if (uri.isEmpty()) continue;
                LspLocation l;
                l.filePath = filePathFromUri(uri);
                const QJsonObject start = o.value("range").toObject().value("start").toObject();
                l.line   = start.value("line").toInt();
                l.column = start.value("character").toInt();
                out.push_back(l);
            }
            cb(out);
        });
}

void LspClient::requestDocumentSymbols(const QString& filePath, DocumentSymbolCallback cb)
{
    if (!cb) return;
    auto fit = m_files.find(filePath);
    if (fit == m_files.end()) { cb({}); return; }
    ServerEntry* srv = m_serversByLang.value(fit->languageId, nullptr);
    if (!srv) { cb({}); return; }

    QByteArray params = "{\"textDocument\":{\"uri\":";
    params += jsonEscape(fileUri(filePath));
    params += "}}";

    const QString fp = filePath;
    sendRequest(srv, "textDocument/documentSymbol", params,
        [cb, fp](const QJsonValue& result, bool isError) {
            QList<LspSymbol> out;
            if (isError || !result.isArray()) { cb(out); return; }
            // The server may return either DocumentSymbol[] (hierarchical, has
            // `selectionRange`) or SymbolInformation[] (flat, has `location`).
            // We disabled hierarchicalDocumentSymbolSupport in initialize, so
            // most servers will reply with SymbolInformation[].
            for (const QJsonValue& v : result.toArray()) {
                const QJsonObject o = v.toObject();
                LspSymbol s;
                s.name          = o.value("name").toString();
                s.containerName = o.value("containerName").toString();
                s.kind          = o.value("kind").toInt(0);
                s.filePath      = fp;
                if (o.contains("location")) {
                    const QJsonObject loc = o.value("location").toObject();
                    const QString uri = loc.value("uri").toString();
                    if (!uri.isEmpty()) s.filePath = filePathFromUri(uri);
                    const QJsonObject start = loc.value("range").toObject().value("start").toObject();
                    s.line   = start.value("line").toInt();
                    s.column = start.value("character").toInt();
                } else {
                    // DocumentSymbol shape: prefer selectionRange over range.
                    const QJsonObject range = o.contains("selectionRange")
                                                  ? o.value("selectionRange").toObject()
                                                  : o.value("range").toObject();
                    const QJsonObject start = range.value("start").toObject();
                    s.line   = start.value("line").toInt();
                    s.column = start.value("character").toInt();
                }
                if (!s.name.isEmpty()) out.push_back(s);
            }
            cb(out);
        });
}

void LspClient::requestWorkspaceSymbols(const QString& query, WorkspaceSymbolCallback cb)
{
    if (!cb) return;
    // Pick any running server to talk to. Workspace symbols are per-server
    // anyway, so iterating multiple servers and merging would be overkill —
    // the active document's language is the most useful default.
    if (m_serversByLang.isEmpty()) { cb({}); return; }
    ServerEntry* srv = m_serversByLang.values().first();
    if (!srv) { cb({}); return; }

    QByteArray params = "{\"query\":";
    params += jsonEscape(query);
    params += "}";

    sendRequest(srv, "workspace/symbol", params,
        [cb](const QJsonValue& result, bool isError) {
            QList<LspSymbol> out;
            if (isError || !result.isArray()) { cb(out); return; }
            for (const QJsonValue& v : result.toArray()) {
                const QJsonObject o = v.toObject();
                LspSymbol s;
                s.name          = o.value("name").toString();
                s.containerName = o.value("containerName").toString();
                s.kind          = o.value("kind").toInt(0);
                const QJsonObject loc = o.value("location").toObject();
                s.filePath = filePathFromUri(loc.value("uri").toString());
                const QJsonObject start = loc.value("range").toObject().value("start").toObject();
                s.line   = start.value("line").toInt();
                s.column = start.value("character").toInt();
                if (!s.name.isEmpty()) out.push_back(s);
            }
            cb(out);
        });
}

void LspClient::requestRename(const QString& filePath, int line, int column,
                              const QString& newName, RenameCallback cb)
{
    if (!cb) return;
    auto fit = m_files.find(filePath);
    if (fit == m_files.end()) { cb({}); return; }
    ServerEntry* srv = m_serversByLang.value(fit->languageId, nullptr);
    if (!srv) { cb({}); return; }

    QByteArray params = buildPositionParams(fileUri(filePath), line, column, &jsonEscape);
    if (params.endsWith('}')) {
        params.chop(1);
        params += ",\"newName\":";
        params += jsonEscape(newName);
        params += "}";
    }

    sendRequest(srv, "textDocument/rename", params,
        [cb](const QJsonValue& result, bool isError) {
            QList<LspTextEdit> out;
            if (!isError && result.isObject()) {
                out = parseWorkspaceEdit(result.toObject());
            }
            cb(out);
        });
}

// ---------------------------------------------------------------------------
// M11: code actions / inlay hints
// ---------------------------------------------------------------------------
namespace {
// Hoisted out so the public-API code-action handler can reuse it.
} // namespace

// (forward declared above-namespace so requestRename's lambda can call it.)
QList<LspTextEdit> LspClient::parseWorkspaceEdit(const QJsonObject& we)
{
    QList<LspTextEdit> out;
    if (we.contains("changes")) {
        const QJsonObject changes = we.value("changes").toObject();
        for (auto it = changes.begin(); it != changes.end(); ++it) {
            const QString uri = it.key();
            const QString filePath = filePathFromUri(uri);
            for (const QJsonValue& v : it.value().toArray()) {
                const QJsonObject e = v.toObject();
                LspTextEdit te;
                te.filePath = filePath;
                te.newText  = e.value("newText").toString();
                rangeIntoEdit(e.value("range").toObject(), te);
                out.push_back(te);
            }
        }
    } else if (we.contains("documentChanges")) {
        for (const QJsonValue& docVal : we.value("documentChanges").toArray()) {
            const QJsonObject doc = docVal.toObject();
            const QString uri = doc.value("textDocument").toObject().value("uri").toString();
            const QString filePath = filePathFromUri(uri);
            for (const QJsonValue& v : doc.value("edits").toArray()) {
                const QJsonObject e = v.toObject();
                LspTextEdit te;
                te.filePath = filePath;
                te.newText  = e.value("newText").toString();
                rangeIntoEdit(e.value("range").toObject(), te);
                out.push_back(te);
            }
        }
    }
    return out;
}

void LspClient::requestCodeActions(const QString& filePath, int line,
                                   CodeActionCallback cb)
{
    if (!cb) return;
    auto fit = m_files.find(filePath);
    if (fit == m_files.end()) { cb({}); return; }
    ServerEntry* srv = m_serversByLang.value(fit->languageId, nullptr);
    if (!srv) { cb({}); return; }

    // Range = full active line (start col 0 → next-line col 0). Diagnostics
    // we already received for that line are forwarded so servers can decide
    // which quickfixes apply.
    QByteArray diagsArr = "[";
    bool first = true;
    const QList<LspDiagnostic>& diags = m_diagnostics.value(filePath);
    for (const LspDiagnostic& d : diags) {
        if (d.line != line) continue;
        if (!first) diagsArr += ',';
        first = false;
        diagsArr += "{\"range\":{\"start\":{\"line\":";
        diagsArr += QByteArray::number(d.line);
        diagsArr += ",\"character\":";
        diagsArr += QByteArray::number(d.column);
        diagsArr += "},\"end\":{\"line\":";
        diagsArr += QByteArray::number(d.line);
        diagsArr += ",\"character\":";
        diagsArr += QByteArray::number(d.column + 1);
        diagsArr += "}},\"severity\":1,\"message\":";
        diagsArr += jsonEscape(d.message);
        diagsArr += "}";
    }
    diagsArr += "]";

    QByteArray params;
    params  = "{\"textDocument\":{\"uri\":";
    params += jsonEscape(fileUri(filePath));
    params += "},\"range\":{\"start\":{\"line\":";
    params += QByteArray::number(line);
    params += ",\"character\":0},\"end\":{\"line\":";
    params += QByteArray::number(line + 1);
    params += ",\"character\":0}},\"context\":{\"diagnostics\":";
    params += diagsArr;
    params += "}}";

    sendRequest(srv, "textDocument/codeAction", params,
        [cb](const QJsonValue& result, bool isError) {
            QList<LspCodeAction> out;
            if (isError || !result.isArray()) { cb(out); return; }
            for (const QJsonValue& v : result.toArray()) {
                if (!v.isObject()) continue;
                const QJsonObject o = v.toObject();
                LspCodeAction a;
                a.title = o.value("title").toString();
                a.kind  = o.value("kind").toString();
                if (o.contains("edit") && o.value("edit").isObject()) {
                    a.edits = parseWorkspaceEdit(o.value("edit").toObject());
                } else if (o.contains("command") && !o.contains("edit")) {
                    // Pure-command actions need server-specific execution we
                    // don't implement; surface them as informational entries.
                    a.isCommandOnly = true;
                }
                if (!a.title.isEmpty()) out.push_back(a);
            }
            cb(out);
        });
}

void LspClient::requestSemanticTokens(const QString& filePath, SemanticTokensCallback cb)
{
    if (!cb) return;
    auto fit = m_files.find(filePath);
    if (fit == m_files.end()) { cb({}); return; }
    ServerEntry* srv = m_serversByLang.value(fit->languageId, nullptr);
    if (!srv) { cb({}); return; }

    QByteArray params;
    params  = "{\"textDocument\":{\"uri\":";
    params += jsonEscape(fileUri(filePath));
    params += "}}";

    // Capture the legend by value so the lambda is self-contained even if the
    // server entry disappears (rare, but happens on shutdown races).
    const QStringList legend = srv->semanticTokenTypes;
    sendRequest(srv, "textDocument/semanticTokens/full", params,
        [cb, legend](const QJsonValue& result, bool isError) {
            QList<LspSemanticToken> out;
            if (isError || !result.isObject()) { cb(out); return; }
            const QJsonArray data = result.toObject().value("data").toArray();
            // LSP wire format: groups of 5 ints per token.
            //   deltaLine, deltaStart, length, tokenType, tokenModifiers
            // Lines accumulate; columns reset to deltaStart on a new line, or
            // accumulate when deltaLine == 0.
            int line = 0;
            int col  = 0;
            for (int i = 0; i + 4 < data.size(); i += 5) {
                const int dl  = data.at(i).toInt();
                const int ds  = data.at(i + 1).toInt();
                const int len = data.at(i + 2).toInt();
                const int tt  = data.at(i + 3).toInt();
                if (dl == 0) {
                    col += ds;
                } else {
                    line += dl;
                    col   = ds;
                }
                LspSemanticToken t;
                t.line     = line;
                t.column   = col;
                t.length   = len;
                t.tokenType = (tt >= 0 && tt < legend.size())
                                  ? legend.at(tt)
                                  : QString::number(tt);
                out.push_back(t);
            }
            cb(out);
        });
}

void LspClient::requestInlayHints(const QString& filePath, int startLine, int endLine,
                                  InlayHintCallback cb)
{
    if (!cb) return;
    auto fit = m_files.find(filePath);
    if (fit == m_files.end()) { cb({}); return; }
    ServerEntry* srv = m_serversByLang.value(fit->languageId, nullptr);
    if (!srv) { cb({}); return; }

    QByteArray params;
    params  = "{\"textDocument\":{\"uri\":";
    params += jsonEscape(fileUri(filePath));
    params += "},\"range\":{\"start\":{\"line\":";
    params += QByteArray::number(qMax(0, startLine));
    params += ",\"character\":0},\"end\":{\"line\":";
    params += QByteArray::number(qMax(startLine, endLine));
    params += ",\"character\":0}}}";

    sendRequest(srv, "textDocument/inlayHint", params,
        [cb](const QJsonValue& result, bool isError) {
            QList<LspInlayHint> out;
            if (isError || !result.isArray()) { cb(out); return; }
            for (const QJsonValue& v : result.toArray()) {
                if (!v.isObject()) continue;
                const QJsonObject o = v.toObject();
                LspInlayHint h;
                const QJsonObject pos = o.value("position").toObject();
                h.line   = pos.value("line").toInt();
                h.column = pos.value("character").toInt();
                // `label` may be a plain string or an array of {value, ...}.
                const QJsonValue lab = o.value("label");
                if (lab.isString()) {
                    h.label = lab.toString();
                } else if (lab.isArray()) {
                    QStringList parts;
                    for (const QJsonValue& p : lab.toArray()) {
                        if (p.isString()) parts << p.toString();
                        else              parts << p.toObject().value("value").toString();
                    }
                    h.label = parts.join(QString());
                }
                h.kind         = o.value("kind").toInt(0);
                h.paddingLeft  = o.value("paddingLeft").toBool(false);
                h.paddingRight = o.value("paddingRight").toBool(false);
                if (!h.label.isEmpty()) out.push_back(h);
            }
            cb(out);
        });
}

// ---------------------------------------------------------------------------
// Server lifecycle
// ---------------------------------------------------------------------------

void LspClient::resolveCommandForLanguage(const QString& languageId,
                                          QString& outCmd,
                                          QStringList& outArgs,
                                          QString& outFriendlyName) const
{
    outCmd.clear();
    outArgs.clear();
    outFriendlyName.clear();

    // Defaults.
    if (languageId == "cpp" || languageId == "c") {
        outCmd = "clangd";
        outFriendlyName = "clangd";
    } else if (languageId == "python") {
        outCmd = "pyright-langserver";
        outArgs = QStringList{"--stdio"};
        outFriendlyName = "pyright";
    } else if (languageId == "go") {
        outCmd = "gopls";
        outFriendlyName = "gopls";
    } else if (languageId == "rust") {
        outCmd = "rust-analyzer";
        outFriendlyName = "rust-analyzer";
    } else if (languageId == "typescript" || languageId == "javascript") {
        outCmd = "typescript-language-server";
        outArgs = QStringList{"--stdio"};
        outFriendlyName = "typescript-language-server";
    }

    // QSettings override under [LSP].
    QSettings s("clip52", "notepadpp-qt");
    s.beginGroup("LSP");
    const QString cmdOverride  = s.value(languageId + "/command").toString();
    const QString argsOverride = s.value(languageId + "/args").toString();
    s.endGroup();

    if (!cmdOverride.isEmpty()) outCmd = cmdOverride;
    if (!argsOverride.isEmpty()) outArgs = QProcess::splitCommand(argsOverride);
}

LspClient::ServerEntry* LspClient::ensureServerForLanguage(const QString& languageId)
{
    if (auto it = m_serversByLang.find(languageId); it != m_serversByLang.end()) {
        return it.value();
    }

    QString cmd, friendly;
    QStringList args;
    resolveCommandForLanguage(languageId, cmd, args, friendly);
    if (cmd.isEmpty()) return nullptr;

    // Resolve to absolute path so QProcess error messages are friendlier.
    QString resolvedCmd = QFileInfo::exists(cmd) ? cmd : QStandardPaths::findExecutable(cmd);
    if (resolvedCmd.isEmpty()) {
        QString hint;
        if (cmd == "clangd")
            hint = tr("Servidor %1 não encontrado — instale com: dnf install clang-tools-extra").arg(cmd);
        else if (cmd == "pyright-langserver")
            hint = tr("Servidor %1 não encontrado — instale com: npm install -g pyright").arg(cmd);
        else if (cmd == "gopls")
            hint = tr("Servidor %1 não encontrado — instale com: go install golang.org/x/tools/gopls@latest").arg(cmd);
        else if (cmd == "rust-analyzer")
            hint = tr("Servidor %1 não encontrado — instale com: rustup component add rust-analyzer").arg(cmd);
        else if (cmd == "typescript-language-server")
            hint = tr("Servidor %1 não encontrado — instale com: npm install -g typescript-language-server").arg(cmd);
        else
            hint = tr("Servidor %1 não encontrado no PATH.").arg(cmd);
        emit const_cast<LspClient*>(this)->serverError(friendly, hint);
        return nullptr;
    }

    auto* srv = new ServerEntry();
    srv->languageId  = languageId;
    srv->command     = resolvedCmd;
    srv->args        = args;
    srv->serverName  = friendly;

    auto* proc = new QProcess(this);
    srv->proc = proc;
    proc->setProcessChannelMode(QProcess::SeparateChannels);
    proc->setProperty("lspLanguageId", languageId);

    connect(proc, &QProcess::readyReadStandardOutput,
            this, &LspClient::onProcessReadyRead);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &LspClient::onProcessFinished);
    connect(proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
        onProcessErrorOccurred(static_cast<int>(e));
    });
    connect(proc, &QProcess::readyReadStandardError, this, [proc, friendly, this]() {
        const QByteArray err = proc->readAllStandardError();
        if (!err.isEmpty()) qDebug().noquote() << "[lsp:" << friendly << "stderr]" << err.trimmed();
    });

    m_serversByLang.insert(languageId, srv);

    proc->start(resolvedCmd, args);
    if (!proc->waitForStarted(2000)) {
        emit serverError(friendly, tr("Falha ao iniciar %1: %2").arg(friendly, proc->errorString()));
        m_serversByLang.remove(languageId);
        proc->deleteLater();
        delete srv;
        return nullptr;
    }

    sendInitialize(srv);
    return srv;
}

// ---------------------------------------------------------------------------
// JSON-RPC framing & sending
// ---------------------------------------------------------------------------

void LspClient::sendMessage(ServerEntry* srv, const QByteArray& jsonBody)
{
    if (!srv || !srv->proc || srv->proc->state() != QProcess::Running) return;

    QByteArray header = "Content-Length: ";
    header += QByteArray::number(jsonBody.size());
    header += "\r\n\r\n";
    srv->proc->write(header);
    srv->proc->write(jsonBody);
}

int LspClient::sendRequest(ServerEntry* srv, const QString& method,
                           const QByteArray& paramsJson, ResponseCallback cb)
{
    if (!srv) return 0;
    const int id = srv->nextRequestId++;
    if (cb) srv->pending.insert(id, std::move(cb));

    QByteArray body = "{\"jsonrpc\":\"2.0\",\"id\":";
    body += QByteArray::number(id);
    body += ",\"method\":";
    body += jsonEscape(method);
    body += ",\"params\":";
    body += paramsJson;
    body += "}";

    // Pre-init: only `initialize` is allowed on the wire. Everything else is
    // queued until the initialize response lands and handleIncomingMessage
    // drains queuedAfterInit. We still register the callback above so it
    // fires once the queued bytes are actually sent.
    const bool isInitialize = (method == "initialize");
    if (!srv->initialized && !isInitialize) {
        srv->queuedAfterInit.push_back(body);
    } else {
        sendMessage(srv, body);
    }
    return id;
}

void LspClient::sendNotification(ServerEntry* srv, const QString& method, const QByteArray& paramsJson)
{
    if (!srv) return;

    QByteArray body = "{\"jsonrpc\":\"2.0\",\"method\":";
    body += jsonEscape(method);
    body += ",\"params\":";
    body += paramsJson;
    body += "}";

    // Hold non-init notifications until initialize response lands.
    const bool isHandshake = (method == "initialized" || method == "exit");
    if (!srv->initialized && !isHandshake) {
        srv->queuedAfterInit.push_back(body);
        return;
    }
    sendMessage(srv, body);
}

void LspClient::sendInitialize(ServerEntry* srv)
{
    // We pick a rootUri from the first open file later; for the very first
    // initialize we use the user's current working directory as a placeholder.
    // Most servers tolerate this for diagnostics-only flows.
    QString root = srv->rootUri;
    if (root.isEmpty()) {
        // Find the first opened file (if any) of this language to root from.
        for (auto it = m_files.constBegin(); it != m_files.constEnd(); ++it) {
            if (it->languageId == srv->languageId) {
                root = QUrl::fromLocalFile(rootDirForFile(it.key())).toString();
                break;
            }
        }
        if (root.isEmpty()) root = QUrl::fromLocalFile(QDir::currentPath()).toString();
        srv->rootUri = root;
    }

    QByteArray params;
    params  = "{\"processId\":";
    params += QByteArray::number(QCoreApplication::applicationPid());
    params += ",\"rootUri\":";
    params += jsonEscape(root);
    // Announce the M8 capabilities we actually consume on the client side so
    // servers can tailor their replies (otherwise they default to richer
    // structures we'd just discard).
    params += ",\"capabilities\":{"
             "\"textDocument\":{"
                "\"synchronization\":{\"didSave\":false,\"willSave\":false,\"willSaveWaitUntil\":false},"
                "\"publishDiagnostics\":{\"relatedInformation\":false,\"versionSupport\":false,\"tagSupport\":{\"valueSet\":[1,2]}},"
                "\"hover\":{\"contentFormat\":[\"plaintext\",\"markdown\"]},"
                "\"definition\":{\"linkSupport\":false},"
                "\"completion\":{\"completionItem\":{\"snippetSupport\":false},\"contextSupport\":true},"
                "\"signatureHelp\":{\"signatureInformation\":{\"parameterInformation\":{\"labelOffsetSupport\":false}}},"
                "\"references\":{\"dynamicRegistration\":false},"
                "\"documentSymbol\":{\"hierarchicalDocumentSymbolSupport\":false},"
                "\"rename\":{\"prepareSupport\":false},"
                "\"codeAction\":{\"codeActionLiteralSupport\":{\"codeActionKind\":{\"valueSet\":[\"quickfix\",\"refactor\",\"source\"]}},\"resolveSupport\":{\"properties\":[\"edit\"]}},"
                "\"inlayHint\":{\"resolveSupport\":{\"properties\":[\"label\",\"tooltip\"]}},"
                "\"semanticTokens\":{\"requests\":{\"full\":true},\"tokenTypes\":[\"namespace\",\"type\",\"class\",\"enum\",\"interface\",\"struct\",\"typeParameter\",\"parameter\",\"variable\",\"property\",\"enumMember\",\"event\",\"function\",\"method\",\"macro\",\"keyword\",\"modifier\",\"comment\",\"string\",\"number\",\"regexp\",\"operator\"],\"tokenModifiers\":[\"declaration\",\"definition\",\"readonly\",\"static\",\"deprecated\",\"abstract\",\"async\",\"modification\",\"documentation\",\"defaultLibrary\"],\"formats\":[\"relative\"]}"
             "},"
             "\"workspace\":{\"workspaceFolders\":true,\"configuration\":false,"
                "\"symbol\":{\"dynamicRegistration\":false}"
             "}"
             "},\"trace\":\"off\"}";
             "\"workspace\":{\"workspaceFolders\":true,\"configuration\":false}"
             "},\"trace\":\"off\"}";

    sendRequest(srv, "initialize", params,
        [this, srv](const QJsonValue& result, bool isError) {
            if (isError) {
                emit serverError(srv->serverName, tr("Falha no initialize do LSP."));
                return;
            }
            // M13: capture the semantic-tokens legend so requestSemanticTokens
            // can resolve token types back to names. Servers may omit this if
            // they don't support semanticTokens at all — leaves the list empty.
            const QJsonObject caps = result.toObject().value("capabilities").toObject();
            const QJsonObject stp  = caps.value("semanticTokensProvider").toObject();
            const QJsonObject leg  = stp.value("legend").toObject();
            for (const QJsonValue& v : leg.value("tokenTypes").toArray())
                srv->semanticTokenTypes << v.toString();

            srv->initialized = true;
            sendInitialized(srv);
            // Drain queued post-init messages.
            for (const QByteArray& m : srv->queuedAfterInit) sendMessage(srv, m);
            srv->queuedAfterInit.clear();
        });
}

void LspClient::sendInitialized(ServerEntry* srv)
{
    sendNotification(srv, "initialized", "{}");
}

// ---------------------------------------------------------------------------
// Incoming bytes → messages
// ---------------------------------------------------------------------------

void LspClient::onProcessReadyRead()
{
    auto* proc = qobject_cast<QProcess*>(sender());
    if (!proc) return;
    const QString lang = proc->property("lspLanguageId").toString();
    ServerEntry* srv = m_serversByLang.value(lang, nullptr);
    if (!srv) return;

    srv->buffer += proc->readAllStandardOutput();
    processIncoming(srv);
}

void LspClient::onProcessFinished(int exitCode, int exitStatus)
{
    auto* proc = qobject_cast<QProcess*>(sender());
    if (!proc) return;
    const QString lang = proc->property("lspLanguageId").toString();
    ServerEntry* srv = m_serversByLang.value(lang, nullptr);
    const QString name = srv ? srv->serverName : lang;
    if (exitStatus == QProcess::CrashExit || exitCode != 0) {
        emit serverError(name, tr("Servidor encerrou inesperadamente (exit=%1).").arg(exitCode));
    }
    if (srv) {
        m_serversByLang.remove(lang);
        delete srv;
    }
    proc->deleteLater();
}

void LspClient::onProcessErrorOccurred(int error)
{
    auto* proc = qobject_cast<QProcess*>(sender());
    if (!proc) return;
    const QString lang = proc->property("lspLanguageId").toString();
    ServerEntry* srv = m_serversByLang.value(lang, nullptr);
    const QString name = srv ? srv->serverName : lang;
    emit serverError(name, tr("Erro de processo do LSP (%1): %2").arg(error).arg(proc->errorString()));
}

void LspClient::processIncoming(ServerEntry* srv)
{
    // LSP framing: "Content-Length: N\r\n\r\n" + body. Parse incrementally;
    // a single read may yield zero, partial, or multiple complete messages.
    while (true) {
        const int headerEnd = srv->buffer.indexOf("\r\n\r\n");
        if (headerEnd < 0) return; // header not yet complete

        const QByteArray header = srv->buffer.left(headerEnd);
        int contentLength = -1;
        for (const QByteArray& line : header.split('\n')) {
            const QByteArray trimmed = line.trimmed();
            if (trimmed.startsWith("Content-Length:")) {
                contentLength = trimmed.mid(QByteArrayLiteral("Content-Length:").size()).trimmed().toInt();
            }
        }
        if (contentLength < 0) {
            // Malformed; drop the header and resync.
            srv->buffer.remove(0, headerEnd + 4);
            continue;
        }
        const int total = headerEnd + 4 + contentLength;
        if (srv->buffer.size() < total) return; // body still incoming

        const QByteArray body = srv->buffer.mid(headerEnd + 4, contentLength);
        srv->buffer.remove(0, total);

        handleIncomingMessage(srv, body);
    }
}

void LspClient::handleIncomingMessage(ServerEntry* srv, const QByteArray& body)
{
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qDebug() << "[lsp]" << srv->serverName << "invalid JSON-RPC payload:" << err.errorString();
        return;
    }
    const QJsonObject obj = doc.object();

    // Server → client request (e.g. workspace/configuration). We don't support
    // any of these meaningfully yet; reply with an error so they don't stall.
    if (obj.contains("id") && obj.contains("method")) {
        QByteArray reply = "{\"jsonrpc\":\"2.0\",\"id\":";
        reply += QByteArray::number(static_cast<qint64>(obj.value("id").toDouble()));
        reply += ",\"error\":{\"code\":-32601,\"message\":\"Method not implemented (basic LSP client)\"}}";
        sendMessage(srv, reply);
        return;
    }

    // Response to one of our requests. Route by id.
    if (obj.contains("id") && !obj.contains("method")) {
        const int id = static_cast<int>(obj.value("id").toDouble());
        const bool isError = obj.contains("error");
        const QJsonValue payload = isError ? obj.value("error") : obj.value("result");
        ResponseCallback cb = srv->pending.take(id);
        if (cb) cb(payload, isError);
        return;
    }

    // Notification.
    const QString method = obj.value("method").toString();
    if (method == "textDocument/publishDiagnostics") {
        const QJsonObject params = obj.value("params").toObject();
        const QString uri = params.value("uri").toString();
        const QString filePath = filePathFromUri(uri);
        if (filePath.isEmpty()) return;

        QList<LspDiagnostic> diags;
        const QJsonArray arr = params.value("diagnostics").toArray();
        diags.reserve(arr.size());
        for (const QJsonValue& v : arr) {
            const QJsonObject d = v.toObject();
            LspDiagnostic out;
            out.message = d.value("message").toString();
            const int sev = d.value("severity").toInt(1);
            switch (sev) {
                case 1: out.severity = "error";   break;
                case 2: out.severity = "warning"; break;
                case 3: out.severity = "info";    break;
                case 4: out.severity = "hint";    break;
                default: out.severity = "info";   break;
            }
            const QJsonObject range = d.value("range").toObject();
            const QJsonObject start = range.value("start").toObject();
            out.line   = start.value("line").toInt();
            out.column = start.value("character").toInt();
            out.source = d.value("source").toString(srv->serverName);

            const QJsonValue codeVal = d.value("code");
            if (codeVal.isString())      out.code = codeVal.toString();
            else if (codeVal.isDouble()) out.code = QString::number(static_cast<qint64>(codeVal.toDouble()));

            diags.push_back(out);
        }

        m_diagnostics.insert(filePath, diags);
        emit diagnosticsUpdated(filePath, diags);
        return;
    }

    // Other notifications (window/showMessage, $/progress, ...) — ignored in basic client.
}
