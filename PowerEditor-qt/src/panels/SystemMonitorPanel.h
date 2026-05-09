#pragma once

#include <QDockWidget>

class QTreeWidget;
class QLabel;
class QPushButton;
class QLineEdit;
class QTimer;

// SystemMonitorPanel (M62) — top-like via /proc, sem deps externas.
class SystemMonitorPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit SystemMonitorPanel(QWidget* parent = nullptr);
public slots:
    void refresh();
private slots:
    void onKill();
    void onTogglePoll();
private:
    QLabel*       m_summary = nullptr;
    QLineEdit*    m_filter  = nullptr;
    QTreeWidget*  m_procs   = nullptr;
    QPushButton*  m_killBtn = nullptr;
    QPushButton*  m_pollBtn = nullptr;
    QPushButton*  m_refreshBtn = nullptr;
    QTimer*       m_timer = nullptr;
};
