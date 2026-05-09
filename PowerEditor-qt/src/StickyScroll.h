#pragma once

#include <QObject>
#include <QPointer>
#include <QHash>
#include <QList>
#include <QString>

class ScintillaEdit;
class QLabel;
class QWidget;

// StickyScroll — draws a "header" of parent-context lines on top of the
// editor (one row per nesting level). The parent lines are derived from
// Scintilla's fold levels; whatever the lexer marked as a header
// (`SC_FOLDLEVELHEADERFLAG`) and is currently scrolled out of view becomes
// a sticky row.
//
// Implementation: a borderless QLabel widget per level, parented to the
// editor's viewport, repositioned on every visible-line change. Click on a
// row scrolls the editor to that header.
class StickyScroll : public QObject {
    Q_OBJECT
public:
    explicit StickyScroll(QObject* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

public:
    // Attach to an editor. Detaches from any previously attached editor.
    // Pass nullptr to disable.
    void setActiveEditor(ScintillaEdit* editor);

    // Master toggle. Persisted in QSettings ("StickyScroll/Enabled").
    void setEnabled(bool on);
    bool isEnabled() const { return m_enabled; }

    // Maximum number of nested headers shown (default 5).
    void setMaxLevels(int n);

private slots:
    void onUpdateUi(int updated);
    void onEditorResized();

private:
    void rebuildLabels();
    void layoutLabels();
    void clearLabels();

    QList<int> activeHeaderLines() const;   // ordered outermost → innermost

    QPointer<ScintillaEdit> m_editor;
    QList<QLabel*>          m_labels;
    bool                    m_enabled   = true;
    int                     m_maxLevels = 5;
};
