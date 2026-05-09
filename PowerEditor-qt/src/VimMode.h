#pragma once

#include <QObject>
#include <QSet>
#include <QString>
#include <QByteArray>
#include <QPointer>

class ScintillaEdit;
class QKeyEvent;

// VimMode (M14) — minimal-but-useful Vim emulator on top of ScintillaEdit.
//
// Supported:
//   Modes: Normal / Insert / Visual.
//   Motion: h j k l, w b, 0 $ ^, gg G, e (end of word).
//   Operators: d (delete), y (yank), c (change). With dd / yy / cc and
//   composed forms (dw, yw, d$, y0, …). x, X, p, P.
//   Edit: i a o O I A, u (undo), Ctrl+R (redo), J (join line).
//   Visual: v starts char-wise visual; d/y/c/x apply to selection.
//   Ex commands: :w :q :wq :x :e <file> :s/old/new/[g]
//
// Not supported (out of scope for first iteration): registers other than
// the unnamed one, marks, search (/), text objects (iw/ip/etc.), macros,
// counts longer than three digits, visual-line/block.
class VimMode : public QObject {
    Q_OBJECT
public:
    enum class Mode { Normal, Insert, Visual };

    explicit VimMode(QObject* parent = nullptr);

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool on);

    void attach(ScintillaEdit* editor);
    void detach(ScintillaEdit* editor);

    Mode mode() const { return m_mode; }
    QString modeLabel() const;

signals:
    void modeChanged(VimMode::Mode);
    // Emitted by `:w` / `:wq` / `:x` so MainWindow can run saveTab.
    void requestSaveCurrentTab();
    // Emitted by `:q` / `:wq` / `:x`.
    void requestCloseCurrentTab();
    // Emitted by `:e <file>` so MainWindow can openFile.
    void requestOpenFile(const QString& path);
    // Emitted on every command (op or motion) so a status-bar widget can
    // surface what's being typed.
    void commandPending(const QString& text);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    bool handleKey(ScintillaEdit* sci, QKeyEvent* event);
    void handleNormal(ScintillaEdit* sci, QKeyEvent* event, bool& consumed);
    void handleVisual(ScintillaEdit* sci, QKeyEvent* event, bool& consumed);

    // Returns the target byte position of `motion` (count `count`) starting
    // from `from`, or -1 if the motion isn't recognized.
    int  motionTarget(ScintillaEdit* sci, int from, char motion, int count);

    void enterMode(Mode m);
    void resetPending();
    int  effectiveCount() const { return m_pendingCount > 0 ? m_pendingCount : 1; }

    void applyOperator(ScintillaEdit* sci, char op, int from, int to,
                       bool linewise);
    void doYank(ScintillaEdit* sci, int from, int to, bool linewise);
    void doDelete(ScintillaEdit* sci, int from, int to, bool linewise);
    void doPaste(ScintillaEdit* sci, bool after);

    void runExCommand(ScintillaEdit* sci, const QString& cmd);

    bool   m_enabled = false;
    Mode   m_mode    = Mode::Normal;
    char   m_pendingOp = 0;        // 'd' 'y' 'c' or 0
    int    m_pendingCount = 0;     // digit accumulator
    bool   m_gPending = false;     // first 'g' seen — wait for second
    int    m_visualAnchor = -1;    // byte offset where visual mode started

    QByteArray m_register;
    bool       m_registerLinewise = false;

    QSet<ScintillaEdit*> m_editors;
};
