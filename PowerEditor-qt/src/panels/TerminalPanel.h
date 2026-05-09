#pragma once

#include <QDockWidget>
#include <QPointer>

class QPlainTextEdit;
class QSocketNotifier;
class QToolButton;
class QLineEdit;

// TerminalPanel — minimal interactive shell embedded as a dock widget.
//
// Implementation note: real terminal emulation (vt100 / xterm sequences,
// alternate screens, proper cursor positioning, scroll regions) is a
// project of its own. For M9 we settle for a "log + input line" model:
//   * spawn a shell with forkpty(3) on Linux
//   * pipe its stdout/stderr into a read-only QPlainTextEdit
//   * read input from a separate QLineEdit and write it (followed by '\n')
//     into the master fd
//   * strip ANSI CSI sequences before display (so colored prompts don't
//     show as "\x1b[1;32m")
//
// This is enough to run shell commands, scripts, even short interactive
// sessions; full-screen TUIs (vim, htop) won't render correctly. That
// trade is fine for the editor's "show me the output" use case.
class TerminalPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit TerminalPanel(QWidget* parent = nullptr);
    ~TerminalPanel() override;

    // Start (or restart) the default shell ($SHELL or /bin/bash).
    void startShell(const QString& shell = QString());

    // Start an arbitrary command instead of the default shell. Used by the
    // SSH client to spawn `ssh user@host` in the same terminal widget.
    // The first argv element should be the executable name.
    void startCommand(const QString& executable, const QStringList& args,
                      const QString& windowTitle = QString());

    // Send a line of text to the shell, appending '\n'.
    void sendLine(const QString& text);

    // Stop the shell process (best-effort SIGHUP).
    void stopShell();

protected:
    void closeEvent(QCloseEvent* e) override;

private slots:
    void onPtyReadable();
    void onSubmit();
    void onClearClicked();
    void onRestartClicked();

private:
    void appendOutput(const QByteArray& bytes);
    static QByteArray stripAnsi(const QByteArray& in);

    QPlainTextEdit* m_view  = nullptr;
    QLineEdit*      m_input = nullptr;
    QToolButton*    m_clear = nullptr;
    QToolButton*    m_restart = nullptr;

    int             m_masterFd = -1;
    int             m_childPid = -1;
    QPointer<QSocketNotifier> m_notifier;
};
