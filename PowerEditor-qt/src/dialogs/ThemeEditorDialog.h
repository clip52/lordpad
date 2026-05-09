#pragma once

#include <QDialog>
#include <QPointer>
#include <QString>
#include <QHash>

class ScintillaEdit;
class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QLabel;

// ThemeEditorDialog (M14) — visual color picker for the most-used Scintilla
// style categories (default, comment, string, number, keyword, identifier,
// operator, preprocessor, line number). Edits apply live to the bound editor.
//
// We map each logical category to lexer-specific style numbers using a small
// table. Languages we know about: cpp/c, python, javascript/typescript,
// markdown, json/yaml, generic (uses STYLE_DEFAULT only).
//
// Persistence: per-lexer in QSettings under "ThemeEditor/<lexerName>/<cat>/{fore,back}".
class ThemeEditorDialog : public QDialog {
    Q_OBJECT
public:
    ThemeEditorDialog(ScintillaEdit* editor, const QString& lexerName,
                      QWidget* parent = nullptr);

private slots:
    void onPickFore();
    void onPickBack();
    void onApplyAll();
    void onResetCategory();

private:
    enum Column { Col_Category = 0, Col_Fore, Col_Back };
    void rebuild();
    void persistCategory(const QString& categoryKey);
    void applyColors(const QString& categoryKey, int fore, int back);
    QList<int> stylesForCategory(const QString& categoryKey) const;
    void loadPersisted(const QString& categoryKey, int& fore, int& back) const;

    QPointer<ScintillaEdit> m_editor;
    QString                 m_lexerName;
    QTreeWidget*            m_tree = nullptr;
    QPushButton*            m_pickFore = nullptr;
    QPushButton*            m_pickBack = nullptr;
    QPushButton*            m_applyAll = nullptr;
    QPushButton*            m_reset    = nullptr;
    QLabel*                 m_status   = nullptr;
};
