#include "TimeTrackerPanel.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

TimeTrackerPanel::TimeTrackerPanel(QWidget* parent) : QDockWidget(tr("Time tracker"), parent)
{
    setObjectName(QStringLiteral("TimeTrackerPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);
    auto* root = new QWidget(this);
    m_project = new QLineEdit(root);
    m_project->setPlaceholderText(tr("Nome do projeto"));
    m_btn = new QPushButton(tr("Iniciar"), root);
    m_clearBtn = new QPushButton(tr("Limpar histórico"), root);
    m_running = new QLabel(tr("(parado)"), root);
    m_running->setStyleSheet(QStringLiteral("QLabel { font-weight: bold; }"));
    m_log = new QTreeWidget(root);
    m_log->setHeaderLabels({ tr("Projeto"), tr("Início"), tr("Duração") });
    m_log->setRootIsDecorated(false);
    m_log->header()->setStretchLastSection(true);

    auto* row = new QHBoxLayout();
    row->addWidget(m_project, 1);
    row->addWidget(m_btn);
    row->addWidget(m_clearBtn);
    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row);
    lay->addWidget(m_running);
    lay->addWidget(m_log, 1);
    setWidget(root);

    m_elapsed = new QElapsedTimer();
    m_tick = new QTimer(this); m_tick->setInterval(1000);
    connect(m_tick, &QTimer::timeout, this, &TimeTrackerPanel::onTick);
    connect(m_btn, &QPushButton::clicked, this, &TimeTrackerPanel::onStartStop);
    connect(m_clearBtn, &QPushButton::clicked, this, &TimeTrackerPanel::onClearAll);

    load();
}

TimeTrackerPanel::~TimeTrackerPanel() { delete m_elapsed; }

void TimeTrackerPanel::onStartStop()
{
    if (!m_active) {
        m_currentProject = m_project->text().trimmed();
        if (m_currentProject.isEmpty()) return;
        m_elapsed->start();
        m_active = true;
        m_btn->setText(tr("Parar"));
        m_running->setText(tr("Em curso: %1").arg(m_currentProject));
        m_tick->start();
    } else {
        const qint64 ms = m_elapsed->elapsed();
        m_active = false;
        m_btn->setText(tr("Iniciar"));
        m_running->setText(tr("(parado)"));
        m_tick->stop();
        // Append entry.
        QSettings s;
        QStringList rows = s.value(QStringLiteral("TimeTracker/log")).toStringList();
        rows << QStringLiteral("%1\x1F%2\x1F%3")
                    .arg(m_currentProject,
                         QDateTime::currentDateTime().toString(Qt::ISODate),
                         QString::number(ms));
        s.setValue(QStringLiteral("TimeTracker/log"), rows);
        rebuild();
    }
}

void TimeTrackerPanel::onTick()
{
    if (!m_active) return;
    const qint64 sec = m_elapsed->elapsed() / 1000;
    m_running->setText(tr("Em curso: %1   %2:%3:%4")
        .arg(m_currentProject)
        .arg(sec / 3600, 2, 10, QLatin1Char('0'))
        .arg((sec / 60) % 60, 2, 10, QLatin1Char('0'))
        .arg(sec % 60, 2, 10, QLatin1Char('0')));
}

void TimeTrackerPanel::onClearAll()
{
    QSettings s; s.remove(QStringLiteral("TimeTracker/log"));
    rebuild();
}

void TimeTrackerPanel::rebuild()
{
    m_log->clear();
    QSettings s;
    const QStringList rows = s.value(QStringLiteral("TimeTracker/log")).toStringList();
    for (const QString& r : rows) {
        const QStringList parts = r.split(QChar(0x1F));
        if (parts.size() < 3) continue;
        const qint64 ms = parts[2].toLongLong();
        const qint64 sec = ms / 1000;
        const QString dur = QStringLiteral("%1h%2m%3s")
            .arg(sec / 3600).arg((sec / 60) % 60).arg(sec % 60);
        auto* it = new QTreeWidgetItem(m_log);
        it->setText(0, parts[0]);
        it->setText(1, parts[1]);
        it->setText(2, dur);
    }
}

void TimeTrackerPanel::load() { rebuild(); }
void TimeTrackerPanel::persist() const {}
