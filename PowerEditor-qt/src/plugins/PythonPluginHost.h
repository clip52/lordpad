#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>

#include <functional>

class ScintillaEdit;
struct _object;        // PyObject — forward-declared to keep this header free of <Python.h>
typedef struct _object PyObject;

// PythonPluginHost — embeds a CPython interpreter and exposes a `notepadpp`
// module that user-supplied .py files in ~/.config/notepadpp-qt/plugins/ can
// import to drive the editor.
//
// Build-time: when CMake doesn't find python3-embed, NPP_HAVE_PYTHON is
// undefined and every method becomes a no-op (isAvailable() returns false).
//
// Threading: every call into the interpreter runs on the Qt main thread.
// We don't release the GIL anywhere — there are no plugin-spawned threads.
class PythonPluginHost : public QObject {
    Q_OBJECT
public:
    explicit PythonPluginHost(QObject* parent = nullptr);
    ~PythonPluginHost() override;

    // True iff this binary was built with python3-embed (NPP_HAVE_PYTHON).
    bool isAvailable() const;

    // Boot the interpreter, register the `notepadpp` module, evaluate the
    // prelude that builds the editor / buffer / ui sub-namespaces. Idempotent.
    bool initialize();

    // Stop the interpreter. Called from MainWindow::closeEvent. Idempotent.
    void shutdown();

    // Singleton accessor for the binding C functions to find the host.
    static PythonPluginHost* instance();

    // ---- Plugin lifecycle -------------------------------------------------

    // Returns absolute paths of *.py files under the user plugin dir,
    // sorted alphabetically. Creates the dir if missing.
    QStringList discoverPlugins() const;

    // The directory plugins live in. Always points to
    // QStandardPaths::AppConfigLocation/plugins.
    QString pluginDir() const;

    // Load (or re-load) a plugin file. On error returns false and writes a
    // human-readable message into *error.
    bool loadPlugin(const QString& path, QString* error = nullptr);

    // Unload a previously loaded plugin (best-effort: drops the module ref +
    // detaches its hooks; Python doesn't fully support GC of imported modules).
    void unloadPlugin(const QString& path);

    // Re-discover and re-load every plugin file. Used by the manager dialog.
    void reloadAll();

    // List of currently loaded plugin paths.
    QStringList loadedPlugins() const;

    // Last error reported for `path`, empty when none.
    QString errorFor(const QString& path) const;

    // ---- Wiring back into the editor -------------------------------------

    // The host is decoupled from MainWindow via std::function callbacks: when
    // a plugin calls notepadpp.editor.text() the binding C function asks the
    // host for the active editor; the host returns whatever the accessor
    // produces. MainWindow installs all of these once at startup.
    using SciAccessor    = std::function<ScintillaEdit*()>;
    using PathAccessor   = std::function<QString()>;
    using SaveAccessor   = std::function<bool()>;
    using LexerAccessor  = std::function<QString()>;
    using MsgHandler     = std::function<void(const QString& title, const QString& text)>;
    using AskHandler     = std::function<QString(const QString& prompt, const QString& def, bool* ok)>;
    using ActionHandler  = std::function<void(const QString& label, std::function<void()> callback)>;

    void setSciAccessor(SciAccessor f);
    void setPathAccessor(PathAccessor f);
    void setSaveAccessor(SaveAccessor f);
    void setLexerAccessor(LexerAccessor f);
    void setMessageHandler(MsgHandler f);
    void setAskHandler(AskHandler f);
    void setActionHandler(ActionHandler f);

    // ---- Event dispatch ---------------------------------------------------

    // Called by MainWindow when something happens; we invoke every registered
    // Python callback. Errors raised inside Python are caught and logged so a
    // misbehaving plugin can't kill the editor.
    void emitOnSave(const QString& path);
    void emitOnLoad(const QString& path);
    void emitOnTextChanged(const QString& path);
    void emitOnTabChanged(const QString& path);

    // ---- Internals exposed to the C binding functions --------------------
    // (Public because the binding fns are free functions in a translation unit
    //  outside this class. They route every call through these helpers so the
    //  callbacks stay in one place.)

    ScintillaEdit* activeEditor() const;
    QString        activeFilePath() const;
    QString        activeLexerName() const;
    bool           saveActive();
    void           showInfoMessage(const QString& title, const QString& text);
    QString        askInputDialog(const QString& prompt, const QString& def, bool* ok);
    void           registerMenuAction(const QString& label, PyObject* callable);
    void           registerHook(const QString& kind, PyObject* callable);

signals:
    // The dialog listens to refresh after reloads.
    void pluginsChanged();

private:
    struct Plugin {
        QString   path;
        PyObject* module = nullptr;
        QString   error;
    };

    void runPrelude();                     // setup notepadpp.editor / buffer / ui
    void clearHooksFor(const QString& path) const;
    void invokeHookList(const QString& kind, PyObject* args);
    bool execFile(const QString& path, PyObject** outModule, QString* error);

    // Registered-from-Python state. Stored as PyObject* with INCREFs balanced
    // in shutdown() / unloadPlugin().
    struct HookEntry { PyObject* callable = nullptr; QString pluginPath; };
    QHash<QString, QList<HookEntry>> m_hooks;   // kind ("on_save", "on_load", ...) -> entries

    QHash<QString, Plugin> m_plugins;
    QString m_currentlyLoading;   // set during execFile() so hook registration can attribute

    SciAccessor    m_sci;
    PathAccessor   m_path;
    SaveAccessor   m_save;
    LexerAccessor  m_lexer;
    MsgHandler     m_msg;
    AskHandler     m_ask;
    ActionHandler  m_action;

    bool m_initialized = false;
};
