#include "FormatOnSave.h"

#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>

#include <ScintillaEdit.h>

namespace {
constexpr const char* kEnabledKey = "FormatOnSave/Enabled";
}

FormatOnSave::FormatOnSave(QObject* parent) : QObject(parent)
{
    seedDefaults();
    load();
}

void FormatOnSave::seedDefaults()
{
    auto add = [this](const QString& lexer, const QString& cmd, const QStringList& args) {
        Config c; c.command = cmd; c.args = args;
        m_configs.insert(lexer, c);
    };
    // Each formatter reads stdin and emits stdout — same contract for all.
    add(QStringLiteral("cpp"),    QStringLiteral("clang-format"), {});
    add(QStringLiteral("c"),      QStringLiteral("clang-format"), {});
    add(QStringLiteral("python"), QStringLiteral("black"),
        { QStringLiteral("-q"), QStringLiteral("-") });
    add(QStringLiteral("go"),     QStringLiteral("gofmt"), {});
    add(QStringLiteral("rust"),   QStringLiteral("rustfmt"),
        { QStringLiteral("--emit"), QStringLiteral("stdout"),
          QStringLiteral("--quiet"), QStringLiteral("--edition"), QStringLiteral("2021") });
    // prettier needs the filename so it picks the right parser. We use the
    // ${file} placeholder; FormatOnSave::applyIfEnabled substitutes at run time.
    add(QStringLiteral("javascript"), QStringLiteral("prettier"),
        { QStringLiteral("--stdin-filepath"), QStringLiteral("${file}") });
    add(QStringLiteral("typescript"), QStringLiteral("prettier"),
        { QStringLiteral("--stdin-filepath"), QStringLiteral("${file}") });
    add(QStringLiteral("json"),       QStringLiteral("prettier"),
        { QStringLiteral("--stdin-filepath"), QStringLiteral("${file}") });
    add(QStringLiteral("yaml"),       QStringLiteral("prettier"),
        { QStringLiteral("--stdin-filepath"), QStringLiteral("${file}") });
    add(QStringLiteral("html"),       QStringLiteral("prettier"),
        { QStringLiteral("--stdin-filepath"), QStringLiteral("${file}") });
    add(QStringLiteral("css"),        QStringLiteral("prettier"),
        { QStringLiteral("--stdin-filepath"), QStringLiteral("${file}") });
    add(QStringLiteral("markdown"),   QStringLiteral("prettier"),
        { QStringLiteral("--stdin-filepath"), QStringLiteral("${file}") });
}

void FormatOnSave::load()
{
    QSettings s;
    m_enabled = s.value(kEnabledKey, false).toBool();
    s.beginGroup(QStringLiteral("FormatOnSave"));
    const QStringList groups = s.childGroups();
    for (const QString& lexer : groups) {
        s.beginGroup(lexer);
        Config c;
        c.command = s.value(QStringLiteral("command")).toString();
        c.args    = s.value(QStringLiteral("args")).toStringList();
        if (!c.command.isEmpty()) m_configs.insert(lexer, c);
        s.endGroup();
    }
    s.endGroup();
}

void FormatOnSave::setEnabled(bool on)
{
    if (m_enabled == on) return;
    m_enabled = on;
    QSettings s; s.setValue(kEnabledKey, on);
}

FormatOnSave::Config FormatOnSave::configFor(const QString& lexerName) const
{
    return m_configs.value(lexerName);
}

void FormatOnSave::setConfig(const QString& lexerName, const Config& cfg)
{
    if (cfg.command.isEmpty()) {
        m_configs.remove(lexerName);
        QSettings s;
        s.beginGroup(QStringLiteral("FormatOnSave/") + lexerName);
        s.remove(QString());
        s.endGroup();
        return;
    }
    m_configs.insert(lexerName, cfg);
    QSettings s;
    s.beginGroup(QStringLiteral("FormatOnSave/") + lexerName);
    s.setValue(QStringLiteral("command"), cfg.command);
    s.setValue(QStringLiteral("args"),    cfg.args);
    s.endGroup();
}

bool FormatOnSave::applyIfEnabled(ScintillaEdit* editor, const QString& lexerName,
                                  const QString& filePath, QString* outErr)
{
    if (!m_enabled || !editor) return false;
    const Config cfg = configFor(lexerName);
    if (!cfg.valid()) return false;

    // Resolve the formatter executable so we can fail fast with a clear msg.
    const QString resolved = QStandardPaths::findExecutable(cfg.command);
    if (resolved.isEmpty()) {
        if (outErr) *outErr = tr("Formatter '%1' não encontrado no PATH.").arg(cfg.command);
        return false;
    }

    QStringList args;
    args.reserve(cfg.args.size());
    for (QString a : cfg.args) {
        a.replace(QStringLiteral("${file}"), filePath);
        args << a;
    }

    QProcess p;
    p.start(resolved, args);
    if (!p.waitForStarted(2000)) {
        if (outErr) *outErr = tr("Falha ao iniciar formatter."); return false;
    }
    QByteArray bufBytes = editor->getText(editor->textLength() + 1);
    p.write(bufBytes);
    p.closeWriteChannel();
    if (!p.waitForFinished(15000)) {
        p.kill(); p.waitForFinished(500);
        if (outErr) *outErr = tr("Formatter demorou demais."); return false;
    }
    if (p.exitCode() != 0) {
        if (outErr) *outErr = QString::fromUtf8(p.readAllStandardError()).trimmed();
        return false;
    }
    const QByteArray out = p.readAllStandardOutput();
    if (out.isEmpty() || out == bufBytes) return true;   // nothing to do

    // Apply within a single undo group so Ctrl+Z reverts the whole thing.
    editor->beginUndoAction();
    editor->setText(out.constData());
    editor->endUndoAction();
    return true;
}
