#include "TimerPanel.h"

#include <QApplication>
#include <QFontDatabase>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace {
QString fmtMs(qint64 ms)
{
    const qint64 totalSec = ms / 1000;
    const qint64 mm = totalSec / 60;
    const qint64 ss = totalSec % 60;
    const qint64 hh = mm / 60;
    const qint64 m  = mm % 60;
    if (hh > 0) return QString::asprintf("%02lld:%02lld:%02lld", hh, m, ss);
    return QString::asprintf("%02lld:%02lld.%03lld", m, ss, ms % 1000);
}
QString fmtCountdown(qint64 ms)
{
    if (ms < 0) ms = 0;
    const qint64 totalSec = ms / 1000;
    return QString::asprintf("%02lld:%02lld", totalSec / 60, totalSec % 60);
}
}

TimerPanel::TimerPanel(QWidget* parent) : QDockWidget(tr("Tempo"), parent)
{
    setObjectName(QStringLiteral("TimerPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);

    // Stopwatch
    auto* swBox = new QGroupBox(tr("Cronômetro"), root);
    QFont big = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    big.setPointSize(20); big.setBold(true);
    m_swDisplay  = new QLabel(QStringLiteral("00:00.000"), swBox);
    m_swDisplay->setFont(big);
    m_swDisplay->setAlignment(Qt::AlignCenter);
    m_swStartBtn = new QPushButton(tr("Iniciar"), swBox);
    m_swResetBtn = new QPushButton(tr("Zerar"), swBox);
    auto* swRow = new QHBoxLayout();
    swRow->addWidget(m_swStartBtn); swRow->addWidget(m_swResetBtn);
    auto* swLay = new QVBoxLayout(swBox);
    swLay->addWidget(m_swDisplay);
    swLay->addLayout(swRow);

    // Pomodoro
    auto* pBox = new QGroupBox(tr("Pomodoro"), root);
    m_focusMin     = new QSpinBox(pBox); m_focusMin->setRange(1, 120); m_focusMin->setValue(25);
    m_breakMin     = new QSpinBox(pBox); m_breakMin->setRange(1, 60);  m_breakMin->setValue(5);
    m_longBreakMin = new QSpinBox(pBox); m_longBreakMin->setRange(1, 60); m_longBreakMin->setValue(15);
    m_cycles       = new QSpinBox(pBox); m_cycles->setRange(1, 12);    m_cycles->setValue(4);
    m_pomoStartBtn = new QPushButton(tr("Iniciar foco"), pBox);
    m_pomoDisplay  = new QLabel(QStringLiteral("--:--"), pBox);
    m_pomoDisplay->setFont(big); m_pomoDisplay->setAlignment(Qt::AlignCenter);
    m_pomoPhase    = new QLabel(tr("ocioso"), pBox);
    m_pomoPhase->setAlignment(Qt::AlignCenter);
    auto* pConfig = new QHBoxLayout();
    pConfig->addWidget(new QLabel(tr("Foco:"), pBox));      pConfig->addWidget(m_focusMin);
    pConfig->addWidget(new QLabel(tr("Pausa:"), pBox));     pConfig->addWidget(m_breakMin);
    pConfig->addWidget(new QLabel(tr("Pausa longa:"), pBox)); pConfig->addWidget(m_longBreakMin);
    pConfig->addWidget(new QLabel(tr("Ciclos:"), pBox));    pConfig->addWidget(m_cycles);
    pConfig->addStretch(1);
    auto* pLay = new QVBoxLayout(pBox);
    pLay->addLayout(pConfig);
    pLay->addWidget(m_pomoDisplay);
    pLay->addWidget(m_pomoPhase);
    pLay->addWidget(m_pomoStartBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addWidget(swBox);
    lay->addWidget(pBox);
    lay->addStretch(1);
    setWidget(root);

    m_swTimer = new QTimer(this); m_swTimer->setInterval(50);
    m_pomoTimer = new QTimer(this); m_pomoTimer->setInterval(250);

    connect(m_swStartBtn, &QPushButton::clicked, this, &TimerPanel::onStartStopwatch);
    connect(m_swResetBtn, &QPushButton::clicked, this, &TimerPanel::onResetStopwatch);
    connect(m_swTimer,    &QTimer::timeout,      this, &TimerPanel::onTickStopwatch);
    connect(m_pomoStartBtn, &QPushButton::clicked, this, &TimerPanel::onStartPomodoro);
    connect(m_pomoTimer,  &QTimer::timeout,      this, &TimerPanel::onTickPomodoro);
}

void TimerPanel::onStartStopwatch()
{
    if (!m_swRunning) {
        m_swElapsed.restart();
        m_swRunning = true;
        m_swStartBtn->setText(tr("Pausar"));
        m_swTimer->start();
    } else {
        m_swPaused += m_swElapsed.elapsed();
        m_swRunning = false;
        m_swStartBtn->setText(tr("Continuar"));
        m_swTimer->stop();
    }
}
void TimerPanel::onResetStopwatch()
{
    m_swElapsed.invalidate();
    m_swPaused = 0;
    m_swRunning = false;
    m_swStartBtn->setText(tr("Iniciar"));
    m_swTimer->stop();
    m_swDisplay->setText(QStringLiteral("00:00.000"));
}
void TimerPanel::onTickStopwatch()
{
    qint64 ms = m_swPaused;
    if (m_swRunning && m_swElapsed.isValid()) ms += m_swElapsed.elapsed();
    m_swDisplay->setText(fmtMs(ms));
}

QString TimerPanel::phaseLabel(PomoPhase p) const
{
    switch (p) {
        case PomoPhase::Idle:       return tr("ocioso");
        case PomoPhase::Focus:      return tr("FOCO");
        case PomoPhase::ShortBreak: return tr("pausa");
        case PomoPhase::LongBreak:  return tr("pausa longa");
    }
    return {};
}
void TimerPanel::enterPhase(PomoPhase p)
{
    m_phase = p;
    m_pomoPhase->setText(phaseLabel(p));
    m_pomoElapsed.restart();
    if (p == PomoPhase::Idle) {
        m_pomoTimer->stop();
        m_pomoDisplay->setText(QStringLiteral("--:--"));
        m_pomoStartBtn->setText(tr("Iniciar foco"));
        m_completedFocus = 0;
    } else {
        m_pomoTimer->start();
    }
}
void TimerPanel::onStartPomodoro()
{
    if (m_phase == PomoPhase::Idle) enterPhase(PomoPhase::Focus);
    else                            enterPhase(PomoPhase::Idle);
}
void TimerPanel::onTickPomodoro()
{
    qint64 totalMs = 0;
    if (m_phase == PomoPhase::Focus)         totalMs = qint64(m_focusMin->value())     * 60 * 1000;
    else if (m_phase == PomoPhase::ShortBreak) totalMs = qint64(m_breakMin->value())   * 60 * 1000;
    else if (m_phase == PomoPhase::LongBreak)  totalMs = qint64(m_longBreakMin->value()) * 60 * 1000;
    else return;

    const qint64 remaining = totalMs - m_pomoElapsed.elapsed();
    m_pomoDisplay->setText(fmtCountdown(remaining));
    if (remaining <= 0) {
        QApplication::beep();
        emit notify(tr("Pomodoro: %1 terminada").arg(phaseLabel(m_phase)));
        if (m_phase == PomoPhase::Focus) {
            ++m_completedFocus;
            if (m_completedFocus >= m_cycles->value()) enterPhase(PomoPhase::LongBreak);
            else                                       enterPhase(PomoPhase::ShortBreak);
        } else if (m_phase == PomoPhase::LongBreak) {
            enterPhase(PomoPhase::Idle);
        } else {
            enterPhase(PomoPhase::Focus);
        }
    }
}
