#pragma once

#include <QDockWidget>
#include <QHash>
#include <QString>

class QTreeWidget;
class QTreeWidgetItem;
class QTextEdit;
class QToolBar;
class QAction;
class QLineEdit;
class QSplitter;
class QTimer;
class QLabel;

// NotesPanel (M14) — CherryTree-style hierarchical notebook with a built-in
// rich-text editor.
//
// Layout: a QSplitter with (1) a QTreeWidget of notes on the left and (2) a
// rich-text editor + formatting toolbar on the right. Clicking a node loads
// its body into the editor; edits are debounced (500 ms) and persisted to a
// single JSON file at ~/.config/notepadpp-qt/cherry-notes.json.
//
// Tree operations exposed via toolbar buttons:
//   New child, New sibling, Rename, Delete, Move up, Move down.
// Rich-text operations:
//   Bold, Italic, Underline, Strikethrough, H1/H2/H3, Bullet list,
//   Numbered list, Foreground color, Insert link, Code block.
//
// The editor portion can stand alone as a "rich editor" — open the panel,
// pick the root node, and just type. CherryTree adds the tree on top.
class NotesPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit NotesPanel(QWidget* parent = nullptr);
    ~NotesPanel() override;

private slots:
    void onTreeSelectionChanged();
    void onBodyChanged();
    void onFlushPending();

    void onNewChild();
    void onNewSibling();
    void onRename();
    void onDelete();
    void onMoveUp();
    void onMoveDown();

    void onBold();
    void onItalic();
    void onUnderline();
    void onStrike();
    void onHeading();           // QAction carries the level in data()
    void onBulletList();
    void onNumberedList();
    void onForeColor();
    void onInsertLink();
    void onCodeBlock();

private:
    QString notesFilePath() const;
    void loadFromDisk();
    void saveToDisk() const;

    QTreeWidgetItem* makeNode(QTreeWidgetItem* parent, const QString& title,
                              const QString& html);
    QTreeWidgetItem* currentNode() const;
    void persistCurrentBody();

    // (de)serialize one tree branch to/from QJsonObject.
    class QJsonObject* serialize(QTreeWidgetItem* it) const;
    void deserialize(const class QJsonObject& obj, QTreeWidgetItem* parent);

    QSplitter*    m_split = nullptr;
    QTreeWidget*  m_tree  = nullptr;
    QTextEdit*    m_body  = nullptr;
    QToolBar*     m_fmtBar = nullptr;
    QLineEdit*    m_titleEdit = nullptr;
    QLabel*       m_status = nullptr;
    QTimer*       m_saveTimer = nullptr;

    bool m_suppressBodySignal = false;
    bool m_suppressTreeSignal = false;
};
