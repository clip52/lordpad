#pragma once

#include <QDockWidget>
#include <QString>
#include <QStringList>

class QTableWidget;
class QPushButton;
class QLabel;
class QPlainTextEdit;
class ScintillaEdit;

// MdTablePanel (M34) — edita tabelas markdown visualmente.
//   * Parse → carrega tabela do buffer ativo (busca primeiro `|...|` block)
//   * Edit → tabela QTableWidget; Enter aplica
//   * Generate → reescreve em markdown padding-aligned
class MdTablePanel : public QDockWidget {
    Q_OBJECT
public:
    explicit MdTablePanel(QWidget* parent = nullptr);

    void setActiveEditor(ScintillaEdit* editor);

private slots:
    void onParse();
    void onAddRow();
    void onAddCol();
    void onWriteBack();

private:
    QString renderMarkdown() const;

    ScintillaEdit*  m_editor = nullptr;
    QTableWidget*   m_table = nullptr;
    QPlainTextEdit* m_preview = nullptr;
    QPushButton*    m_parseBtn = nullptr;
    QPushButton*    m_addRowBtn = nullptr;
    QPushButton*    m_addColBtn = nullptr;
    QPushButton*    m_writeBtn = nullptr;
    QLabel*         m_status = nullptr;
    int             m_blockStart = -1;   // line numbers in the source
    int             m_blockEnd   = -1;
};
