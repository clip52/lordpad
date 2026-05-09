#pragma once

#include <QDockWidget>
#include <QPointer>
#include <QString>
#include <QList>

class ScintillaEdit;
class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QLabel;

// MergeResolverPanel — locates `<<<<<<<` / `=======` / `>>>>>>>` blocks in
// the active editor and offers per-conflict accept-ours / accept-theirs /
// keep-both buttons. Diff3-style three-way blocks (with `|||||||` between
// `<<<<<<<` and `=======`) are also recognized; the base side is shown but
// only acts as context — accept buttons collapse the conflict to the chosen
// side(s) only.
//
// The panel is read-mostly: the buffer is the source of truth, and a
// "Atualizar" button re-scans on demand. We don't watch text changes
// continuously because the resolver is an explicit step, not background
// noise.
class MergeResolverPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit MergeResolverPanel(QWidget* parent = nullptr);

    void setActiveEditor(ScintillaEdit* editor);

signals:
    // Emitted when an accept action mutates the buffer; MainWindow uses this
    // to surface a status-bar message.
    void resolutionApplied(int conflictsLeft);

private slots:
    void onScan();
    void onAcceptOurs();
    void onAcceptTheirs();
    void onKeepBoth();
    void onItemActivated(QTreeWidgetItem* it, int col);

private:
    struct Conflict {
        // All offsets are byte positions inside the buffer.
        int startMarkerLine = 0;        // line of `<<<<<<<`
        int separatorLine   = 0;        // line of `=======`
        int endMarkerLine   = 0;        // line of `>>>>>>>`
        int baseSeparatorLine = -1;     // line of `|||||||` (diff3) or -1
        QString ours;                   // text between <<<<<<< (excl) and ======= or |||||||
        QString theirs;                 // text between ======= and >>>>>>>
    };
    QList<Conflict> findConflicts(ScintillaEdit* sci) const;

    void applyResolution(int conflictIndex, const QString& replacement);
    Conflict* selectedConflict();

    QPointer<ScintillaEdit> m_editor;
    QList<Conflict>         m_conflicts;

    QTreeWidget* m_tree = nullptr;
    QLabel*      m_status = nullptr;
    QPushButton* m_btnRescan = nullptr;
    QPushButton* m_btnOurs = nullptr;
    QPushButton* m_btnTheirs = nullptr;
    QPushButton* m_btnBoth = nullptr;
};
