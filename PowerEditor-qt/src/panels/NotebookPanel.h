#pragma once

#include <QDockWidget>
#include <QString>
#include <QStringList>
#include <QList>

class QPlainTextEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QComboBox;
class QLabel;

// NotebookPanel (M14) — minimal Jupyter notebook (.ipynb) viewer/editor.
//
// Cells: each one is { type: "code"|"markdown", source: text }. The list on
// the left holds the cell sequence; the right pane edits the active cell's
// source. "Run cell" pipes the source to `python -c -` (for code cells) and
// shows stdout/stderr below the editor.
//
// Persistence: open() loads from disk, save()/saveAs() writes a valid v4
// notebook JSON back. Outputs aren't rendered or persisted in this MVP.
class NotebookPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit NotebookPanel(QWidget* parent = nullptr);

    bool openFile(const QString& path);
    bool save();
    bool saveAs();

private slots:
    void onCellSelectionChanged();
    void onSourceChanged();
    void onAddCode();
    void onAddMarkdown();
    void onDeleteCell();
    void onMoveUp();
    void onMoveDown();
    void onRunCell();
    void onOpen();
    void onSave();

private:
    struct Cell {
        QString type;       // "code" | "markdown"
        QString source;     // raw text (newline-joined)
        QString lastOutput; // captured stdout from the latest run
    };

    void rebuildList();
    void persistCurrentSource();
    QListWidgetItem* itemForRow(int row) const;
    int currentRow() const;
    static QString cellHeadline(const Cell& c);

    QString      m_filePath;
    QList<Cell>  m_cells;

    QListWidget*    m_list = nullptr;
    QComboBox*      m_typeCombo = nullptr;
    QPlainTextEdit* m_source = nullptr;
    QPlainTextEdit* m_output = nullptr;
    QPushButton*    m_runBtn = nullptr;
    QPushButton*    m_addCodeBtn = nullptr;
    QPushButton*    m_addMdBtn   = nullptr;
    QPushButton*    m_delBtn     = nullptr;
    QPushButton*    m_upBtn      = nullptr;
    QPushButton*    m_downBtn    = nullptr;
    QPushButton*    m_openBtn    = nullptr;
    QPushButton*    m_saveBtn    = nullptr;
    QLabel*         m_status = nullptr;

    bool m_suppressSourceSignal = false;
};
