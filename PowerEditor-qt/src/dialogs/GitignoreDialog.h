#pragma once

#include <QDialog>
#include <QString>

class QPlainTextEdit;
class QListWidget;

// GitignoreDialog (M19) — edita .gitignore do repo + insere templates comuns.
class GitignoreDialog : public QDialog {
    Q_OBJECT
public:
    GitignoreDialog(const QString& anchorFilePath, QWidget* parent = nullptr);

private slots:
    void onSave();
    void onInsertTemplate();

private:
    QString         m_path;
    QPlainTextEdit* m_text = nullptr;
    QListWidget*    m_templates = nullptr;
};
