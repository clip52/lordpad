#pragma once

#include <QDockWidget>
#include <QPointer>

class QPushButton;
class QTreeWidget;
class QPlainTextEdit;
class QLineEdit;
class QLabel;
class QProcess;

// DockerPanel (M54) — listar containers, logs, exec via subprocess docker.
class DockerPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit DockerPanel(QWidget* parent = nullptr);
private slots:
    void onRefresh();
    void onLogs();
    void onExec();
    void onStop();
    void onStart();
private:
    QString currentContainerId() const;

    QPushButton*    m_refreshBtn = nullptr;
    QPushButton*    m_logsBtn    = nullptr;
    QPushButton*    m_execBtn    = nullptr;
    QPushButton*    m_startBtn   = nullptr;
    QPushButton*    m_stopBtn    = nullptr;
    QLineEdit*      m_execCmd    = nullptr;
    QTreeWidget*    m_list       = nullptr;
    QPlainTextEdit* m_out        = nullptr;
    QLabel*         m_status     = nullptr;
    QPointer<QProcess> m_logsProc;
};
