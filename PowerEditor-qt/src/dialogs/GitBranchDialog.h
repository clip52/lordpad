#pragma once

#include <QDialog>
#include <QString>

class QListWidget;
class QListWidgetItem;
class QPushButton;
class QCheckBox;
class QLabel;
class QLineEdit;

// GitBranchDialog — minimal branch picker (M12). Lists local + (optionally)
// remote branches, marks the current one, and exposes Checkout / Create-from
// actions. Heavy operations (rebase, push/pull, fetch) live in the terminal.
class GitBranchDialog : public QDialog {
    Q_OBJECT
public:
    GitBranchDialog(const QString& anchorFilePath, QWidget* parent = nullptr);

private slots:
    void onCheckout();
    void onCreate();
    void onIncludeRemoteToggled();
    void onItemDoubleClicked(QListWidgetItem* it);

private:
    void rebuild();

    QString      m_anchor;
    QListWidget* m_list = nullptr;
    QCheckBox*   m_remoteChk = nullptr;
    QLineEdit*   m_newName = nullptr;
    QPushButton* m_checkoutBtn = nullptr;
    QPushButton* m_createBtn = nullptr;
    QLabel*      m_status = nullptr;
};
