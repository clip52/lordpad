#pragma once

#include <QDockWidget>
#include <QString>

class QLineEdit;
class QPushButton;
class QTreeWidget;
class QLabel;
class QTimer;
class QElapsedTimer;

// TimeTrackerPanel (M39) — start/stop por projeto, persiste sessões.
class TimeTrackerPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit TimeTrackerPanel(QWidget* parent = nullptr);
    ~TimeTrackerPanel() override;
private slots:
    void onStartStop();
    void onClearAll();
    void onTick();
private:
    void rebuild();
    void load();
    void persist() const;
    QLineEdit*    m_project = nullptr;
    QPushButton*  m_btn = nullptr;
    QPushButton*  m_clearBtn = nullptr;
    QTreeWidget*  m_log = nullptr;
    QLabel*       m_running = nullptr;
    QString       m_currentProject;
    QElapsedTimer* m_elapsed = nullptr;
    QTimer*       m_tick = nullptr;
    bool          m_active = false;
};
