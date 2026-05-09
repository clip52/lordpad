#pragma once

#include <QDockWidget>
#include <QPointer>

class QLineEdit;
class QPushButton;
class QPlainTextEdit;
class QLabel;
class QComboBox;
class QProcess;

// SshExecPanel (M59) — exec rápido em host SSH (usa ssh CLI / config).
class SshExecPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit SshExecPanel(QWidget* parent = nullptr);
private slots:
    void onRun();
    void onSaveHost();
    void onClear();
private:
    void loadHosts();

    QComboBox*      m_host = nullptr;
    QLineEdit*      m_cmd  = nullptr;
    QPushButton*    m_runBtn = nullptr;
    QPushButton*    m_saveBtn = nullptr;
    QPushButton*    m_clearBtn = nullptr;
    QPlainTextEdit* m_out = nullptr;
    QLabel*         m_status = nullptr;
    QPointer<QProcess> m_proc;
};
