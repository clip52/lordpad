#pragma once

#include <QDockWidget>
#include <QPointer>

class QComboBox;
class QPushButton;
class QTreeWidget;
class QPlainTextEdit;
class QLineEdit;
class QLabel;
class QProcess;

// KubectlPanel (M61) — kubectl wrapper: lista pods, logs, exec.
class KubectlPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit KubectlPanel(QWidget* parent = nullptr);
private slots:
    void onRefreshNamespaces();
    void onRefreshPods();
    void onLogs();
    void onExec();
    void onDescribe();
    void onDelete();
private:
    QString currentPod() const;
    QString currentNamespace() const;

    QComboBox*      m_ns = nullptr;
    QPushButton*    m_nsRefreshBtn = nullptr;
    QPushButton*    m_refreshBtn = nullptr;
    QPushButton*    m_logsBtn = nullptr;
    QPushButton*    m_execBtn = nullptr;
    QPushButton*    m_descBtn = nullptr;
    QPushButton*    m_delBtn = nullptr;
    QLineEdit*      m_execCmd = nullptr;
    QTreeWidget*    m_pods = nullptr;
    QPlainTextEdit* m_out = nullptr;
    QLabel*         m_status = nullptr;
    QPointer<QProcess> m_logsProc;
};
