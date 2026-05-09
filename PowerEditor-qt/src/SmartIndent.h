#pragma once

#include <QObject>
#include <QSet>

class ScintillaEdit;
class QKeyEvent;

// SmartIndent (M17) — context-aware Enter handler.
//
//   * Always keeps the previous line's leading whitespace on the new line.
//   * If the previous line ends with `{`, `(`, `[`, `:` (Python-style),
//     adds one extra indent level (tab or N spaces depending on settings).
//   * On `}` typed at the start of an indented line, dedents one level.
class SmartIndent : public QObject {
    Q_OBJECT
public:
    explicit SmartIndent(QObject* parent = nullptr);

    void setEnabled(bool on);
    bool isEnabled() const { return m_enabled; }

    void attach(ScintillaEdit* editor);
    void detach(ScintillaEdit* editor);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    bool handleEnter(ScintillaEdit* sci);
    bool handleClosingBrace(ScintillaEdit* sci);

    bool m_enabled = true;
    QSet<ScintillaEdit*> m_editors;
};
