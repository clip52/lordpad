#pragma once

#include <QDockWidget>

class QLineEdit;
class QPushButton;
class QCheckBox;
class QPlainTextEdit;
class QLabel;
class ScintillaEdit;

// GistPanel (M57) — paste buffer/seleção pra GitHub gist via `gh`.
class GistPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit GistPanel(QWidget* parent = nullptr);
    void setActiveEditor(ScintillaEdit* ed) { m_editor = ed; }
private slots:
    void onCreateGist();
    void onListGists();
private:
    ScintillaEdit*  m_editor = nullptr;
    QLineEdit*      m_descEdit = nullptr;
    QLineEdit*      m_filenameEdit = nullptr;
    QCheckBox*      m_publicBox = nullptr;
    QCheckBox*      m_selectionBox = nullptr;
    QPushButton*    m_createBtn = nullptr;
    QPushButton*    m_listBtn = nullptr;
    QPlainTextEdit* m_out = nullptr;
    QLabel*         m_status = nullptr;
};
