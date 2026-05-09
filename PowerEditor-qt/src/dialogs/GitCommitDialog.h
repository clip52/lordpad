#pragma once

#include <QDialog>
#include <QString>

class QPlainTextEdit;
class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QLabel;

// GitCommitDialog — minimal staging + commit UI.
// Lists `git status --porcelain` rows with a checkbox per file. Toggling
// the checkbox calls `git add` / `git reset HEAD` accordingly. The bottom
// pane is a plain message editor; the Commit button runs `git commit -m`.
//
// Anything beyond that (interactive add, partial hunks, amend, signoff) is
// out of scope — users with those needs reach for the terminal panel.
class GitCommitDialog : public QDialog {
    Q_OBJECT
public:
    GitCommitDialog(const QString& filePathInRepo, QWidget* parent = nullptr);

private slots:
    void onItemChanged(QTreeWidgetItem* item, int col);
    void onCommitClicked();
    void onRefresh();

private:
    void rebuildList();
    bool isStaged(const QString& staged, const QString& unstaged) const;

    QString         m_anchorFile;
    QTreeWidget*    m_files = nullptr;
    QPlainTextEdit* m_message = nullptr;
    QPushButton*    m_commitBtn = nullptr;
    QPushButton*    m_refreshBtn = nullptr;
    QLabel*         m_status = nullptr;
    bool            m_suppressItemSignals = false;
};
