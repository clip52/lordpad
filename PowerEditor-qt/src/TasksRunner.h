#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>

// TasksRunner — workspace-relative `.notepadpp/tasks.json` reader (M12).
//
// Format (intentionally a strict subset of VS Code's tasks.json so users can
// reuse what they already have):
//   {
//     "tasks": [
//       { "label": "build",   "command": "make", "args": ["-j8"], "cwd": "${workspaceFolder}" },
//       { "label": "test",    "command": "pytest", "args": ["-x"] }
//     ]
//   }
//
// `${workspaceFolder}` and `${file}` are substituted at exec time.
class TasksRunner : public QObject {
    Q_OBJECT
public:
    struct Task {
        QString     label;
        QString     command;
        QStringList args;
        QString     cwd;          // empty = workspace root
    };

    explicit TasksRunner(QObject* parent = nullptr);

    // Load `<workspace>/.notepadpp/tasks.json`. Returns the parsed task list,
    // or an empty list (and a non-empty *error) if the file is missing/invalid.
    QList<Task> load(const QString& workspaceFolder, QString* error = nullptr) const;

    // Resolve `${workspaceFolder}` / `${file}` placeholders in the task fields
    // and return a copy ready to hand to QProcess.
    Task resolve(const Task& in,
                 const QString& workspaceFolder,
                 const QString& activeFile) const;
};
