#include "LayoutPresets.h"

#include <QMainWindow>
#include <QSettings>

LayoutPresets::LayoutPresets(QMainWindow* host, QObject* parent)
    : QObject(parent), m_host(host) {}

QStringList LayoutPresets::list() const
{
    QSettings s; s.beginGroup(QStringLiteral("Layouts"));
    return s.childGroups();
}

bool LayoutPresets::save(const QString& name)
{
    if (!m_host || name.trimmed().isEmpty()) return false;
    QSettings s; s.beginGroup(QStringLiteral("Layouts/") + name);
    s.setValue(QStringLiteral("geometry"), m_host->saveGeometry());
    s.setValue(QStringLiteral("state"),    m_host->saveState());
    return true;
}

bool LayoutPresets::apply(const QString& name)
{
    if (!m_host) return false;
    QSettings s; s.beginGroup(QStringLiteral("Layouts/") + name);
    if (!s.contains(QStringLiteral("geometry"))) return false;
    m_host->restoreGeometry(s.value(QStringLiteral("geometry")).toByteArray());
    m_host->restoreState   (s.value(QStringLiteral("state")).toByteArray());
    return true;
}

void LayoutPresets::remove(const QString& name)
{
    QSettings s; s.beginGroup(QStringLiteral("Layouts/") + name);
    s.remove(QString());
}
