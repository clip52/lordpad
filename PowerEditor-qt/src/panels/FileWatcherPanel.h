#pragma once

#include <QDockWidget>
#include <QHash>

class QFileSystemWatcher;
class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QPushButton;
class QPlainTextEdit;
class QLabel;

// FileWatcherPanel (M35) — observa paths e roda comandos quando algo muda.
class FileWatcherPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit FileWatcherPanel(QWidget* parent = nullptr);

private slots:
    void onAddRule();
    void onRemoveRule();
    void onTriggered(const QString& path);

private:
    void persist() const;
    void load();

    QFileSystemWatcher* m_watcher = nullptr;
    QHash<QString, QString> m_rules;   // watched path → command
    QListWidget*    m_rulesList = nullptr;
    QLineEdit*      m_pathEdit = nullptr;
    QLineEdit*      m_cmdEdit = nullptr;
    QPushButton*    m_addBtn = nullptr;
    QPushButton*    m_delBtn = nullptr;
    QPlainTextEdit* m_log = nullptr;
    QLabel*         m_status = nullptr;
};
