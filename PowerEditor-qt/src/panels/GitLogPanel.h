#pragma once

#include <QDockWidget>
#include <QPointer>
#include <QString>

class QTreeWidget;
class QTreeWidgetItem;
class QCheckBox;
class QPushButton;
class QPlainTextEdit;

// GitLogPanel — table of recent commits for the active file's repo.
// Toggle "Apenas este arquivo" to limit log to the active path. Double-click
// a row to load `git show <sha>` into the bottom diff pane.
class GitLogPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit GitLogPanel(QWidget* parent = nullptr);

    // Tell the panel which file (anchor for `git -C ...`) to query against.
    // Empty string clears the view.
    void setAnchorFile(const QString& filePath);

    // Re-run the log query against the current anchor.
    void refresh();

private slots:
    void onItemActivated(QTreeWidgetItem* item, int col);
    void onScopeToggled();

private:
    QString m_anchor;

    QTreeWidget*    m_table = nullptr;
    QPlainTextEdit* m_diff  = nullptr;
    QCheckBox*      m_onlyThis = nullptr;
    QPushButton*    m_refreshBtn = nullptr;
};
