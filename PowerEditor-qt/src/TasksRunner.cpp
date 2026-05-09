#include "TasksRunner.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

TasksRunner::TasksRunner(QObject* parent) : QObject(parent) {}

QList<TasksRunner::Task> TasksRunner::load(const QString& workspaceFolder, QString* error) const
{
    QList<Task> out;
    if (workspaceFolder.isEmpty()) {
        if (error) *error = tr("Sem workspace ativo.");
        return out;
    }
    const QString path = QDir(workspaceFolder).absoluteFilePath(QStringLiteral(".notepadpp/tasks.json"));
    QFile f(path);
    if (!f.exists()) {
        if (error) *error = tr("Sem arquivo .notepadpp/tasks.json no workspace.");
        return out;
    }
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) *error = tr("Não consegui abrir %1: %2").arg(path, f.errorString());
        return out;
    }
    QJsonParseError pe{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &pe);
    f.close();
    if (pe.error != QJsonParseError::NoError) {
        if (error) *error = tr("JSON inválido: %1").arg(pe.errorString());
        return out;
    }
    const QJsonArray arr = doc.object().value(QStringLiteral("tasks")).toArray();
    for (const QJsonValue& v : arr) {
        const QJsonObject o = v.toObject();
        Task t;
        t.label   = o.value(QStringLiteral("label")).toString();
        t.command = o.value(QStringLiteral("command")).toString();
        for (const QJsonValue& a : o.value(QStringLiteral("args")).toArray())
            t.args << a.toString();
        t.cwd = o.value(QStringLiteral("cwd")).toString();
        if (t.label.isEmpty() || t.command.isEmpty()) continue;
        out.append(t);
    }
    return out;
}

TasksRunner::Task TasksRunner::resolve(const Task& in,
                                       const QString& workspaceFolder,
                                       const QString& activeFile) const
{
    auto subst = [&](QString s) {
        s.replace(QStringLiteral("${workspaceFolder}"), workspaceFolder);
        s.replace(QStringLiteral("${file}"), activeFile);
        return s;
    };
    Task out;
    out.label   = in.label;
    out.command = subst(in.command);
    for (const QString& a : in.args) out.args << subst(a);
    out.cwd = in.cwd.isEmpty() ? workspaceFolder : subst(in.cwd);
    return out;
}
