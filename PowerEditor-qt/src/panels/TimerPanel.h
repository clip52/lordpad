#pragma once

#include <QDockWidget>
#include <QElapsedTimer>

class QLabel;
class QPushButton;
class QSpinBox;
class QTimer;
class QComboBox;

// TimerPanel (M26) — combined stopwatch + Pomodoro.
//
// Stopwatch: start/stop/reset, mostra mm:ss.000.
// Pomodoro: configurável (foco / pausa / ciclos), bipa via QApplication::beep
// no fim de cada fase. Notifica via status bar do MainWindow (sinal).
class TimerPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit TimerPanel(QWidget* parent = nullptr);

signals:
    void notify(const QString& text);

private slots:
    void onStartStopwatch();
    void onResetStopwatch();
    void onStartPomodoro();
    void onTickStopwatch();
    void onTickPomodoro();

private:
    enum class PomoPhase { Idle, Focus, ShortBreak, LongBreak };
    QString phaseLabel(PomoPhase p) const;
    void enterPhase(PomoPhase p);

    QElapsedTimer m_swElapsed;
    qint64        m_swPaused = 0;
    bool          m_swRunning = false;
    QTimer*       m_swTimer = nullptr;
    QLabel*       m_swDisplay = nullptr;
    QPushButton*  m_swStartBtn = nullptr;
    QPushButton*  m_swResetBtn = nullptr;

    QSpinBox*     m_focusMin = nullptr;
    QSpinBox*     m_breakMin = nullptr;
    QSpinBox*     m_longBreakMin = nullptr;
    QSpinBox*     m_cycles = nullptr;
    QPushButton*  m_pomoStartBtn = nullptr;
    QLabel*       m_pomoDisplay = nullptr;
    QLabel*       m_pomoPhase = nullptr;
    QTimer*       m_pomoTimer = nullptr;
    QElapsedTimer m_pomoElapsed;
    PomoPhase     m_phase = PomoPhase::Idle;
    int           m_completedFocus = 0;
};
