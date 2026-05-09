#pragma once

#include <QDockWidget>
#include <QString>
#include <QStringList>

class QProcess;
class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QComboBox;
class QLabel;
class QLineEdit;

// TestRunnerPanel — detects pytest / jest / cargo test in the active
// workspace, runs them via QProcess, parses the output line-by-line into a
// tree of (suite/file → test → status). Failures carry file:line locations
// when the framework prints them; double-clicking a row opens the file.
//
// Auto-detection rules (workspace root = directory of active file):
//   pytest   pyproject.toml with [tool.pytest], pytest.ini, conftest.py
//   jest     package.json that lists jest in scripts or dependencies
//   cargo    Cargo.toml
class TestRunnerPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit TestRunnerPanel(QWidget* parent = nullptr);
    ~TestRunnerPanel() override;

    // Set the directory used as cwd for runs and for auto-detection.
    void setWorkspaceFolder(const QString& path);

signals:
    void openFileRequested(const QString& path, int line);

private slots:
    void onDetect();
    void onRun();
    void onStop();
    void onStdout();
    void onStderr();
    void onProcessFinished(int exitCode, int exitStatus);
    void onItemActivated(QTreeWidgetItem* it, int col);

private:
    enum class Framework { None, Pytest, Jest, Cargo };

    Framework detectFramework() const;
    QString frameworkLabel(Framework f) const;
    void clearTree();
    void appendBuffered();

    // Parser dispatcher — returns true if the line was understood.
    void parseLine(const QString& line);
    void parsePytestLine(const QString& line);
    void parseJestLine(const QString& line);
    void parseCargoLine(const QString& line);

    // Helper: find or create a top-level node by name, return its item.
    QTreeWidgetItem* suiteNode(const QString& name);

    QString      m_workspaceFolder;
    Framework    m_framework = Framework::None;
    QProcess*    m_proc = nullptr;
    QString      m_stdoutTail;   // partial last line waiting for newline

    int m_passed = 0;
    int m_failed = 0;
    int m_skipped = 0;

    QLabel*      m_status = nullptr;
    QLabel*      m_workspaceLabel = nullptr;
    QComboBox*   m_frameworkCombo = nullptr;
    QLineEdit*   m_extraArgs = nullptr;
    QPushButton* m_detectBtn = nullptr;
    QPushButton* m_runBtn = nullptr;
    QPushButton* m_stopBtn = nullptr;
    QTreeWidget* m_tree = nullptr;
};
