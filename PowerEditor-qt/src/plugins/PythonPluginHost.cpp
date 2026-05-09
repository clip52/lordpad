#include "PythonPluginHost.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#if defined(NPP_HAVE_PYTHON)
// Qt's `slots` / `signals` keyword macros collide with members named the same
// in Python's headers (e.g. `PyType_Slot *slots`). Undef around the include.
#  pragma push_macro("slots")
#  pragma push_macro("signals")
#  undef slots
#  undef signals
#  define PY_SSIZE_T_CLEAN
#  include <Python.h>
#  pragma pop_macro("signals")
#  pragma pop_macro("slots")
#endif

#include <ScintillaEdit.h>

// =============================================================================
// Singleton plumbing — the binding C functions are static functions outside
// the class, so we route them through this thread-local-style instance pointer.
// Embedded interpreter is single-threaded for plugin calls so a plain pointer
// is enough.
// =============================================================================
static PythonPluginHost* s_host = nullptr;

PythonPluginHost* PythonPluginHost::instance() {
    return s_host;
}

#if defined(NPP_HAVE_PYTHON)

// =============================================================================
// Helpers
// =============================================================================
namespace {

// Small RAII guard that turns a raw PyObject* into a steal-on-construction,
// DECREF-on-destruction pointer. Used to keep error paths leak-free.
class PyRef {
public:
    explicit PyRef(PyObject* p = nullptr) : p_(p) {}
    PyRef(const PyRef&) = delete;
    PyRef& operator=(const PyRef&) = delete;
    PyRef(PyRef&& o) noexcept : p_(o.p_) { o.p_ = nullptr; }
    ~PyRef() { Py_XDECREF(p_); }
    PyObject* get() const { return p_; }
    PyObject* release() { auto* q = p_; p_ = nullptr; return q; }
    explicit operator bool() const { return p_ != nullptr; }
private:
    PyObject* p_;
};

// Convert Python str → QString via UTF-8.
QString pyToQString(PyObject* o) {
    if (!o) return {};
    if (!PyUnicode_Check(o)) {
        PyRef s(PyObject_Str(o));
        return s ? pyToQString(s.get()) : QString();
    }
    Py_ssize_t len = 0;
    const char* utf8 = PyUnicode_AsUTF8AndSize(o, &len);
    if (!utf8) { PyErr_Clear(); return {}; }
    return QString::fromUtf8(utf8, static_cast<int>(len));
}

// Format the current Python exception (if any) as a string, then clear it.
QString consumePyError() {
    if (!PyErr_Occurred()) return {};

    PyObject *type = nullptr, *value = nullptr, *tb = nullptr;
    PyErr_Fetch(&type, &value, &tb);
    PyErr_NormalizeException(&type, &value, &tb);
    PyRef tType(type), tValue(value), tTb(tb);

    QString msg;
    if (value) {
        PyRef s(PyObject_Str(value));
        if (s) msg = pyToQString(s.get());
    }
    if (msg.isEmpty() && type) {
        PyRef s(PyObject_GetAttrString(type, "__name__"));
        msg = s ? pyToQString(s.get()) : QStringLiteral("(unknown Python error)");
    }
    return msg;
}

// =============================================================================
// notepadpp.* binding functions
// All return None unless documented otherwise. Every accessor goes through
// the host singleton so MainWindow controls what "active editor" means.
// =============================================================================
PyObject* py_editor_text(PyObject*, PyObject*) {
    auto* host = PythonPluginHost::instance();
    auto* sci = host ? host->activeEditor() : nullptr;
    if (!sci) Py_RETURN_NONE;
    QByteArray bytes = sci->getText(sci->textLength() + 1);
    return PyUnicode_FromStringAndSize(bytes.constData(), bytes.size());
}

PyObject* py_editor_set_text(PyObject*, PyObject* args) {
    const char* s = nullptr;
    if (!PyArg_ParseTuple(args, "s", &s)) return nullptr;
    auto* host = PythonPluginHost::instance();
    auto* sci = host ? host->activeEditor() : nullptr;
    if (sci) sci->setText(s);
    Py_RETURN_NONE;
}

PyObject* py_editor_insert(PyObject*, PyObject* args) {
    const char* s = nullptr;
    if (!PyArg_ParseTuple(args, "s", &s)) return nullptr;
    auto* host = PythonPluginHost::instance();
    auto* sci = host ? host->activeEditor() : nullptr;
    if (sci) sci->replaceSel(s);
    Py_RETURN_NONE;
}

PyObject* py_editor_replace_selection(PyObject* m, PyObject* args) {
    return py_editor_insert(m, args);   // alias — same op
}

PyObject* py_editor_selected_text(PyObject*, PyObject*) {
    auto* host = PythonPluginHost::instance();
    auto* sci = host ? host->activeEditor() : nullptr;
    if (!sci) Py_RETURN_NONE;
    QByteArray sel = sci->getSelText();
    return PyUnicode_FromStringAndSize(sel.constData(), sel.size());
}

PyObject* py_editor_cursor(PyObject*, PyObject*) {
    auto* host = PythonPluginHost::instance();
    auto* sci = host ? host->activeEditor() : nullptr;
    if (!sci) Py_RETURN_NONE;
    const auto pos = sci->currentPos();
    const int line = static_cast<int>(sci->lineFromPosition(pos)) + 1;
    const int col  = static_cast<int>(sci->column(pos)) + 1;
    return Py_BuildValue("(ii)", line, col);
}

PyObject* py_editor_goto_line(PyObject*, PyObject* args) {
    int line = 0;
    if (!PyArg_ParseTuple(args, "i", &line)) return nullptr;
    auto* host = PythonPluginHost::instance();
    auto* sci = host ? host->activeEditor() : nullptr;
    if (sci) {
        sci->gotoLine(line - 1);   // public API is 1-based
        sci->scrollCaret();
    }
    Py_RETURN_NONE;
}

PyObject* py_editor_line_count(PyObject*, PyObject*) {
    auto* host = PythonPluginHost::instance();
    auto* sci = host ? host->activeEditor() : nullptr;
    return PyLong_FromLong(sci ? static_cast<long>(sci->lineCount()) : 0);
}

PyObject* py_editor_line(PyObject*, PyObject* args) {
    int line = 0;
    if (!PyArg_ParseTuple(args, "i", &line)) return nullptr;
    auto* host = PythonPluginHost::instance();
    auto* sci = host ? host->activeEditor() : nullptr;
    if (!sci) Py_RETURN_NONE;
    QByteArray bytes = sci->getLine(line);
    return PyUnicode_FromStringAndSize(bytes.constData(), bytes.size());
}

PyObject* py_buffer_path(PyObject*, PyObject*) {
    auto* host = PythonPluginHost::instance();
    const QString p = host ? host->activeFilePath() : QString();
    return PyUnicode_FromString(p.toUtf8().constData());
}

PyObject* py_buffer_modified(PyObject*, PyObject*) {
    auto* host = PythonPluginHost::instance();
    auto* sci = host ? host->activeEditor() : nullptr;
    return PyBool_FromLong(sci ? sci->modify() : 0);
}

PyObject* py_buffer_save(PyObject*, PyObject*) {
    auto* host = PythonPluginHost::instance();
    return PyBool_FromLong(host ? host->saveActive() : 0);
}

PyObject* py_buffer_lexer(PyObject*, PyObject*) {
    auto* host = PythonPluginHost::instance();
    const QString l = host ? host->activeLexerName() : QString();
    return PyUnicode_FromString(l.toUtf8().constData());
}

PyObject* py_ui_show_message(PyObject*, PyObject* args, PyObject* kwargs) {
    const char* text = nullptr;
    const char* title = "Plugin";
    static const char* kwlist[] = { "text", "title", nullptr };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|s",
                                     const_cast<char**>(kwlist), &text, &title)) {
        return nullptr;
    }
    auto* host = PythonPluginHost::instance();
    if (host) host->showInfoMessage(QString::fromUtf8(title), QString::fromUtf8(text));
    Py_RETURN_NONE;
}

PyObject* py_ui_input(PyObject*, PyObject* args, PyObject* kwargs) {
    const char* prompt = nullptr;
    const char* def = "";
    static const char* kwlist[] = { "prompt", "default", nullptr };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|s",
                                     const_cast<char**>(kwlist), &prompt, &def)) {
        return nullptr;
    }
    auto* host = PythonPluginHost::instance();
    bool ok = false;
    QString out = host ? host->askInputDialog(QString::fromUtf8(prompt),
                                              QString::fromUtf8(def), &ok)
                       : QString();
    if (!ok) Py_RETURN_NONE;
    return PyUnicode_FromString(out.toUtf8().constData());
}

PyObject* py_ui_add_action(PyObject*, PyObject* args) {
    const char* label = nullptr;
    PyObject* callable = nullptr;
    if (!PyArg_ParseTuple(args, "sO", &label, &callable)) return nullptr;
    if (!PyCallable_Check(callable)) {
        PyErr_SetString(PyExc_TypeError, "second argument must be callable");
        return nullptr;
    }
    auto* host = PythonPluginHost::instance();
    if (host) host->registerMenuAction(QString::fromUtf8(label), callable);
    Py_RETURN_NONE;
}

PyObject* py_register_hook_impl(const QString& kind, PyObject* args) {
    PyObject* callable = nullptr;
    if (!PyArg_ParseTuple(args, "O", &callable)) return nullptr;
    if (!PyCallable_Check(callable)) {
        PyErr_SetString(PyExc_TypeError, "argument must be callable");
        return nullptr;
    }
    auto* host = PythonPluginHost::instance();
    if (host) host->registerHook(kind, callable);
    Py_RETURN_NONE;
}
PyObject* py_on_save        (PyObject*, PyObject* a) { return py_register_hook_impl("on_save", a); }
PyObject* py_on_load        (PyObject*, PyObject* a) { return py_register_hook_impl("on_load", a); }
PyObject* py_on_text_changed(PyObject*, PyObject* a) { return py_register_hook_impl("on_text_changed", a); }
PyObject* py_on_tab_changed (PyObject*, PyObject* a) { return py_register_hook_impl("on_tab_changed", a); }

// Module method table.
PyMethodDef NppMethods[] = {
    {"_editor_text",          py_editor_text,          METH_NOARGS,  "Active editor's full text."},
    {"_editor_set_text",      py_editor_set_text,      METH_VARARGS, "Replace the active editor's text."},
    {"_editor_insert",        py_editor_insert,        METH_VARARGS, "Insert at the caret / replace selection."},
    {"_editor_replace_selection", py_editor_replace_selection, METH_VARARGS, "Replace the current selection."},
    {"_editor_selected_text", py_editor_selected_text, METH_NOARGS,  "Currently selected text."},
    {"_editor_cursor",        py_editor_cursor,        METH_NOARGS,  "(line, col) tuple, both 1-based."},
    {"_editor_goto_line",     py_editor_goto_line,     METH_VARARGS, "Move caret to 1-based line."},
    {"_editor_line_count",    py_editor_line_count,    METH_NOARGS,  "Total number of lines."},
    {"_editor_line",          py_editor_line,          METH_VARARGS, "Text of a 0-based line index."},
    {"_buffer_path",          py_buffer_path,          METH_NOARGS,  "Active file path or empty."},
    {"_buffer_modified",      py_buffer_modified,      METH_NOARGS,  "True if active buffer is dirty."},
    {"_buffer_save",          py_buffer_save,          METH_NOARGS,  "Save the active buffer; returns True on success."},
    {"_buffer_lexer",         py_buffer_lexer,         METH_NOARGS,  "Lexer/language name."},
    {"_ui_show_message",      reinterpret_cast<PyCFunction>(py_ui_show_message), METH_VARARGS | METH_KEYWORDS, "Show an information dialog."},
    {"_ui_input",             reinterpret_cast<PyCFunction>(py_ui_input),        METH_VARARGS | METH_KEYWORDS, "Ask for a string; returns None on cancel."},
    {"_ui_add_action",        py_ui_add_action,        METH_VARARGS, "Register an entry under the Plugins menu."},
    {"on_save",               py_on_save,              METH_VARARGS, "Register a hook called after a buffer is saved."},
    {"on_load",               py_on_load,              METH_VARARGS, "Register a hook called after a file is loaded."},
    {"on_text_changed",       py_on_text_changed,      METH_VARARGS, "Register a hook called on every text edit."},
    {"on_tab_changed",        py_on_tab_changed,       METH_VARARGS, "Register a hook called when the active tab changes."},
    {nullptr, nullptr, 0, nullptr},
};

PyModuleDef NppModuleDef = {
    PyModuleDef_HEAD_INIT,
    "notepadpp",
    "Notepad++ Qt plugin API.",
    -1,
    NppMethods,
    nullptr, nullptr, nullptr, nullptr,
};

PyObject* PyInit_notepadpp_module() {
    return PyModule_Create(&NppModuleDef);
}

// Python-side prelude evaluated after the module is created. Wraps the flat
// `_editor_*` / `_buffer_*` / `_ui_*` C functions in nicer namespaces so
// plugin code reads as `notepadpp.editor.text()` instead of `_editor_text()`.
const char* kPrelude = R"PYEOF(
import sys

class _Editor:
    @staticmethod
    def text():                       return sys.modules['notepadpp']._editor_text()
    @staticmethod
    def set_text(s):                  return sys.modules['notepadpp']._editor_set_text(s)
    @staticmethod
    def insert(s):                    return sys.modules['notepadpp']._editor_insert(s)
    @staticmethod
    def replace_selection(s):         return sys.modules['notepadpp']._editor_replace_selection(s)
    @staticmethod
    def selected_text():              return sys.modules['notepadpp']._editor_selected_text()
    @staticmethod
    def cursor():                     return sys.modules['notepadpp']._editor_cursor()
    @staticmethod
    def goto_line(n):                 return sys.modules['notepadpp']._editor_goto_line(n)
    @staticmethod
    def line_count():                 return sys.modules['notepadpp']._editor_line_count()
    @staticmethod
    def line(n):                      return sys.modules['notepadpp']._editor_line(n)

class _Buffer:
    @staticmethod
    def path():                       return sys.modules['notepadpp']._buffer_path()
    @staticmethod
    def modified():                   return sys.modules['notepadpp']._buffer_modified()
    @staticmethod
    def save():                       return sys.modules['notepadpp']._buffer_save()
    @staticmethod
    def lexer():                      return sys.modules['notepadpp']._buffer_lexer()

class _Ui:
    @staticmethod
    def show_message(text, title="Plugin"):
        return sys.modules['notepadpp']._ui_show_message(text, title)
    @staticmethod
    def input(prompt, default=""):
        return sys.modules['notepadpp']._ui_input(prompt, default)
    @staticmethod
    def add_action(label, callback):
        return sys.modules['notepadpp']._ui_add_action(label, callback)

sys.modules['notepadpp'].editor = _Editor()
sys.modules['notepadpp'].buffer = _Buffer()
sys.modules['notepadpp'].ui     = _Ui()
)PYEOF";

} // namespace

#endif // NPP_HAVE_PYTHON

// =============================================================================
// PythonPluginHost — public API
// =============================================================================
PythonPluginHost::PythonPluginHost(QObject* parent) : QObject(parent) {
    s_host = this;
}

PythonPluginHost::~PythonPluginHost() {
    shutdown();
    if (s_host == this) s_host = nullptr;
}

bool PythonPluginHost::isAvailable() const {
#if defined(NPP_HAVE_PYTHON)
    return true;
#else
    return false;
#endif
}

QString PythonPluginHost::pluginDir() const {
    const QString cfg = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir d(cfg + "/plugins");
    if (!d.exists()) d.mkpath(".");
    return d.absolutePath();
}

QStringList PythonPluginHost::discoverPlugins() const {
    QDir d(pluginDir());
    QStringList files = d.entryList({"*.py"}, QDir::Files | QDir::Readable, QDir::Name);
    for (QString& f : files) f = d.absoluteFilePath(f);
    return files;
}

QStringList PythonPluginHost::loadedPlugins() const { return m_plugins.keys(); }

QString PythonPluginHost::errorFor(const QString& path) const {
    auto it = m_plugins.find(path);
    return (it == m_plugins.end()) ? QString() : it->error;
}

#if defined(NPP_HAVE_PYTHON)

bool PythonPluginHost::initialize() {
    if (m_initialized) return true;

    PyImport_AppendInittab("notepadpp", &PyInit_notepadpp_module);

    // Defer site init until after Py_Initialize so we can short-circuit if the
    // platform Python is broken (e.g. broken venv). PyConfig is the modern API.
    PyConfig config;
    PyConfig_InitPythonConfig(&config);
    config.install_signal_handlers = 0;   // don't steal SIGINT from Qt
    PyStatus status = Py_InitializeFromConfig(&config);
    PyConfig_Clear(&config);
    if (PyStatus_Exception(status)) {
        qWarning() << "[plugins] Py_InitializeFromConfig failed";
        return false;
    }

    // Import the module so PyImport_AppendInittab actually fires, then run
    // the prelude that builds editor/buffer/ui sub-namespaces.
    PyRef mod(PyImport_ImportModule("notepadpp"));
    if (!mod) {
        qWarning() << "[plugins] failed to import notepadpp:" << consumePyError();
        return false;
    }
    runPrelude();

    m_initialized = true;
    return true;
}

void PythonPluginHost::runPrelude() {
    PyRef builtins(PyImport_ImportModule("builtins"));
    PyRef globals(PyDict_New());
    if (!globals) { consumePyError(); return; }
    PyDict_SetItemString(globals.get(), "__builtins__", builtins.get());
    PyRef code(Py_CompileString(kPrelude, "<notepadpp-prelude>", Py_file_input));
    if (!code) { qWarning() << "[plugins] prelude compile failed:" << consumePyError(); return; }
    PyRef result(PyEval_EvalCode(code.get(), globals.get(), globals.get()));
    if (!result) { qWarning() << "[plugins] prelude eval failed:" << consumePyError(); }
}

void PythonPluginHost::shutdown() {
    if (!m_initialized) return;

    // Drop module + hook references; Python takes care of the rest.
    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        if (it->module) Py_DECREF(it->module);
    }
    m_plugins.clear();
    for (auto it = m_hooks.begin(); it != m_hooks.end(); ++it) {
        for (HookEntry& e : it.value()) Py_XDECREF(e.callable);
    }
    m_hooks.clear();

    Py_Finalize();
    m_initialized = false;
}

bool PythonPluginHost::execFile(const QString& path, PyObject** outModule, QString* error) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) *error = tr("Não foi possível abrir o plugin: %1").arg(f.errorString());
        return false;
    }
    const QByteArray src = f.readAll();
    f.close();

    QFileInfo fi(path);
    const QString moduleName = QStringLiteral("npp_plugin_") + fi.completeBaseName();

    PyRef code(Py_CompileString(src.constData(),
                                path.toUtf8().constData(),
                                Py_file_input));
    if (!code) {
        if (error) *error = consumePyError();
        return false;
    }

    PyRef module(PyImport_AddModule(moduleName.toUtf8().constData()));
    if (!module) {
        if (error) *error = consumePyError();
        return false;
    }
    // Set __file__ so plugin tracebacks show the source path.
    PyObject_SetAttrString(module.get(), "__file__",
                           PyRef(PyUnicode_FromString(path.toUtf8().constData())).get());
    PyObject* dict = PyModule_GetDict(module.get());

    PyRef result(PyEval_EvalCode(code.get(), dict, dict));
    if (!result) {
        if (error) *error = consumePyError();
        return false;
    }

    *outModule = module.release();
    Py_INCREF(*outModule);   // keep the entry alive past the steal/release dance
    return true;
}

bool PythonPluginHost::loadPlugin(const QString& path, QString* error) {
    if (!m_initialized && !initialize()) {
        if (error) *error = tr("Interpretador Python não disponível.");
        return false;
    }

    // Reload semantics: drop existing entry first so prior hooks don't double-fire.
    unloadPlugin(path);

    // Tell registerHook() which plugin file is responsible for any hooks
    // registered during this eval. We never recurse, so a plain QString is fine.
    m_currentlyLoading = path;

    PyObject* module = nullptr;
    QString err;
    if (!execFile(path, &module, &err)) {
        m_currentlyLoading.clear();
        m_plugins[path] = Plugin{path, nullptr, err};
        if (error) *error = err;
        emit pluginsChanged();
        return false;
    }
    m_plugins[path] = Plugin{path, module, QString()};
    m_currentlyLoading.clear();
    emit pluginsChanged();
    return true;
}

void PythonPluginHost::unloadPlugin(const QString& path) {
    auto it = m_plugins.find(path);
    if (it == m_plugins.end()) return;
    if (it->module) Py_DECREF(it->module);
    m_plugins.erase(it);
    clearHooksFor(path);
    emit pluginsChanged();
}

void PythonPluginHost::clearHooksFor(const QString& path) const {
    auto* self = const_cast<PythonPluginHost*>(this);
    for (auto it = self->m_hooks.begin(); it != self->m_hooks.end(); ++it) {
        QList<HookEntry>& list = it.value();
        for (int i = list.size() - 1; i >= 0; --i) {
            if (list[i].pluginPath == path) {
                Py_XDECREF(list[i].callable);
                list.removeAt(i);
            }
        }
    }
}

void PythonPluginHost::reloadAll() {
    const QStringList existing = m_plugins.keys();
    for (const QString& p : existing) unloadPlugin(p);
    const QStringList files = discoverPlugins();
    for (const QString& f : files) loadPlugin(f);
}

void PythonPluginHost::registerHook(const QString& kind, PyObject* callable) {
    if (!callable) return;
    Py_INCREF(callable);
    // m_currentlyLoading is set by loadPlugin() while execFile() is on the
    // stack — that's the plugin file responsible for whatever module-level
    // hook registration we're seeing. unloadPlugin() uses this attribution
    // to remove the right hooks on reload.
    m_hooks[kind].push_back(HookEntry{ callable, m_currentlyLoading });
}

void PythonPluginHost::registerMenuAction(const QString& label, PyObject* callable) {
    if (!callable || !m_action) return;
    Py_INCREF(callable);
    PyObject* held = callable;   // moved into the lambda below
    m_action(label, [held]() {
        // Invoke the Python callback. Errors are caught and logged.
        PyRef result(PyObject_CallNoArgs(held));
        if (!result) {
            qWarning() << "[plugins] action callback raised:" << consumePyError();
        }
    });
}

void PythonPluginHost::invokeHookList(const QString& kind, PyObject* args) {
    auto it = m_hooks.find(kind);
    if (it == m_hooks.end()) return;
    for (const HookEntry& e : it.value()) {
        if (!e.callable) continue;
        PyRef result(PyObject_Call(e.callable, args, nullptr));
        if (!result) {
            qWarning() << "[plugins]" << kind
                       << "hook raised:" << consumePyError();
        }
    }
}

void PythonPluginHost::emitOnSave(const QString& path) {
    if (!m_initialized) return;
    PyRef args(Py_BuildValue("(s)", path.toUtf8().constData()));
    if (args) invokeHookList("on_save", args.get());
}
void PythonPluginHost::emitOnLoad(const QString& path) {
    if (!m_initialized) return;
    PyRef args(Py_BuildValue("(s)", path.toUtf8().constData()));
    if (args) invokeHookList("on_load", args.get());
}
void PythonPluginHost::emitOnTextChanged(const QString& path) {
    if (!m_initialized) return;
    PyRef args(Py_BuildValue("(s)", path.toUtf8().constData()));
    if (args) invokeHookList("on_text_changed", args.get());
}
void PythonPluginHost::emitOnTabChanged(const QString& path) {
    if (!m_initialized) return;
    PyRef args(Py_BuildValue("(s)", path.toUtf8().constData()));
    if (args) invokeHookList("on_tab_changed", args.get());
}

#else // NPP_HAVE_PYTHON

bool PythonPluginHost::initialize()                      { return false; }
void PythonPluginHost::shutdown()                        {}
bool PythonPluginHost::loadPlugin(const QString&, QString* err) {
    if (err) *err = tr("Suporte a plugins Python não foi compilado.");
    return false;
}
void PythonPluginHost::unloadPlugin(const QString&)      {}
void PythonPluginHost::clearHooksFor(const QString&) const {}
void PythonPluginHost::reloadAll()                       {}
void PythonPluginHost::runPrelude()                      {}
bool PythonPluginHost::execFile(const QString&, PyObject**, QString*) { return false; }
void PythonPluginHost::registerHook(const QString&, PyObject*)        {}
void PythonPluginHost::registerMenuAction(const QString&, PyObject*)  {}
void PythonPluginHost::invokeHookList(const QString&, PyObject*)      {}
void PythonPluginHost::emitOnSave(const QString&)        {}
void PythonPluginHost::emitOnLoad(const QString&)        {}
void PythonPluginHost::emitOnTextChanged(const QString&) {}
void PythonPluginHost::emitOnTabChanged(const QString&)  {}

#endif // NPP_HAVE_PYTHON

// Wiring setters — same on both build flavours.
void PythonPluginHost::setSciAccessor   (SciAccessor f)   { m_sci    = std::move(f); }
void PythonPluginHost::setPathAccessor  (PathAccessor f)  { m_path   = std::move(f); }
void PythonPluginHost::setSaveAccessor  (SaveAccessor f)  { m_save   = std::move(f); }
void PythonPluginHost::setLexerAccessor (LexerAccessor f) { m_lexer  = std::move(f); }
void PythonPluginHost::setMessageHandler(MsgHandler f)    { m_msg    = std::move(f); }
void PythonPluginHost::setAskHandler    (AskHandler f)    { m_ask    = std::move(f); }
void PythonPluginHost::setActionHandler (ActionHandler f) { m_action = std::move(f); }

ScintillaEdit* PythonPluginHost::activeEditor() const   { return m_sci   ? m_sci()   : nullptr; }
QString        PythonPluginHost::activeFilePath() const { return m_path  ? m_path()  : QString(); }
QString        PythonPluginHost::activeLexerName() const { return m_lexer ? m_lexer() : QString(); }
bool           PythonPluginHost::saveActive()           { return m_save  ? m_save()  : false; }

void PythonPluginHost::showInfoMessage(const QString& title, const QString& text) {
    if (m_msg) m_msg(title, text);
}
QString PythonPluginHost::askInputDialog(const QString& prompt, const QString& def, bool* ok) {
    if (m_ask) return m_ask(prompt, def, ok);
    if (ok) *ok = false;
    return {};
}
