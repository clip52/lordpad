#pragma once

#include <QDockWidget>
#include <QPointer>

class QLineEdit;
class QPushButton;
class QCheckBox;
class QTreeWidget;
class QPlainTextEdit;
class QLabel;
class QProcess;

// SystemdServicesPanel (M67) — wrapper systemctl + journalctl tail.
class SystemdServicesPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit SystemdServicesPanel(QWidget* parent = nullptr);
private slots:
    void onRefresh();
    void onStart();
    void onStop();
    void onRestart();
    void onStatus();
    void onJournalTail();
private:
    QString currentUnit() const;
    QStringList ctlArgs() const;

    QLineEdit*      m_filter = nullptr;
    QCheckBox*      m_userBox = nullptr;       // --user
    QPushButton*    m_refreshBtn = nullptr;
    QPushButton*    m_startBtn = nullptr;
    QPushButton*    m_stopBtn = nullptr;
    QPushButton*    m_restartBtn = nullptr;
    QPushButton*    m_statusBtn = nullptr;
    QPushButton*    m_journalBtn = nullptr;
    QTreeWidget*    m_units = nullptr;
    QPlainTextEdit* m_out = nullptr;
    QLabel*         m_status = nullptr;
    QPointer<QProcess> m_journalProc;
};
