#pragma once

#include <QDialog>
#include <QString>

class QListWidget;
class QListWidgetItem;
class QPushButton;
class QCheckBox;
class QLineEdit;
class QLabel;

// GitStashDialog — minimal stash UI (M12). Lists existing stashes and
// supports push (com mensagem opcional, optional include-untracked),
// apply, pop (apply + drop), drop.
class GitStashDialog : public QDialog {
    Q_OBJECT
public:
    GitStashDialog(const QString& anchorFilePath, QWidget* parent = nullptr);

private slots:
    void onPush();
    void onApply();
    void onPop();
    void onDrop();

private:
    void rebuild();
    QString selectedRef() const;

    QString      m_anchor;
    QListWidget* m_list = nullptr;
    QLineEdit*   m_message = nullptr;
    QCheckBox*   m_untracked = nullptr;
    QPushButton* m_pushBtn  = nullptr;
    QPushButton* m_applyBtn = nullptr;
    QPushButton* m_popBtn   = nullptr;
    QPushButton* m_dropBtn  = nullptr;
    QLabel*      m_status   = nullptr;
};
