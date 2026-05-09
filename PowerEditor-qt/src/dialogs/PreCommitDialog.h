#pragma once

#include <QDialog>
#include <QString>

class QCheckBox;
class QPlainTextEdit;
class QLabel;

// PreCommitDialog (M19) — instala um hook .git/hooks/pre-commit que roda
// formatters + linters comuns. O texto do hook é editável; checkboxes só
// inserem snippets.
class PreCommitDialog : public QDialog {
    Q_OBJECT
public:
    PreCommitDialog(const QString& anchorFilePath, QWidget* parent = nullptr);

private slots:
    void onInstall();
    void onAddBlock();

private:
    QString         m_repoRoot;
    QString         m_hookPath;
    QPlainTextEdit* m_script = nullptr;
    QCheckBox*      m_clangFormat = nullptr;
    QCheckBox*      m_black = nullptr;
    QCheckBox*      m_prettier = nullptr;
    QCheckBox*      m_gofmt = nullptr;
    QLabel*         m_status = nullptr;
};
