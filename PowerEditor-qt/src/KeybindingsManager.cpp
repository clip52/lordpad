#include "KeybindingsManager.h"

#include <QAction>
#include <QMenu>
#include <QSettings>
#include <QWidget>

namespace {
constexpr const char* kSettingsGroup = "Keybindings";
}

KeybindingsManager::KeybindingsManager(QObject* parent) : QObject(parent) {}

QString KeybindingsManager::keyForAction(QAction* a)
{
    if (!a) return {};
    QString t = a->text();
    // Strip mnemonics (&File → File, "&&" → "&").
    QString out;
    out.reserve(t.size());
    for (int i = 0; i < t.size(); ++i) {
        const QChar c = t.at(i);
        if (c == QLatin1Char('&')) {
            if (i + 1 < t.size() && t.at(i + 1) == QLatin1Char('&')) {
                out.append('&'); ++i;
            }
            continue;
        }
        out.append(c);
    }
    return out.trimmed();
}

void KeybindingsManager::collectFromHost(QWidget* host)
{
    if (!host) return;
    m_entries.clear();
    // Recursive: catches actions parented to QMenu / QToolBar / submenus.
    const auto actions = host->findChildren<QAction*>(QString(),
        Qt::FindChildrenRecursively);
    for (QAction* a : actions) {
        if (!a) continue;
        if (a->isSeparator()) continue;
        if (a->menu()) continue;             // submenu titles aren't bindable
        const QString key = keyForAction(a);
        if (key.isEmpty()) continue;
        if (m_entries.contains(key)) continue;   // first wins on collisions
        Entry e;
        e.key             = key;
        e.action          = a;
        e.defaultShortcut = a->shortcut();
        e.currentShortcut = a->shortcut();
        m_entries.insert(key, e);
    }
}

void KeybindingsManager::applyPersistedOverrides()
{
    QSettings s;
    s.beginGroup(kSettingsGroup);
    const QStringList keys = s.allKeys();
    s.endGroup();

    for (const QString& k : keys) {
        auto it = m_entries.find(k);
        if (it == m_entries.end() || !it->action) continue;
        QSettings s2;
        s2.beginGroup(kSettingsGroup);
        const QString stored = s2.value(k).toString();
        s2.endGroup();
        const QKeySequence seq = QKeySequence::fromString(stored);
        it->currentShortcut = seq;
        it->action->setShortcut(seq);
    }
}

QList<KeybindingsManager::Entry> KeybindingsManager::entries() const
{
    QList<Entry> out;
    out.reserve(m_entries.size());
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        out.append(it.value());
    }
    std::sort(out.begin(), out.end(),
              [](const Entry& a, const Entry& b) {
                  return a.key.localeAwareCompare(b.key) < 0;
              });
    return out;
}

bool KeybindingsManager::setOverride(const QString& key, const QKeySequence& seq)
{
    auto it = m_entries.find(key);
    if (it == m_entries.end()) return false;

    it->currentShortcut = seq.isEmpty() ? it->defaultShortcut : seq;
    if (it->action) it->action->setShortcut(it->currentShortcut);

    QSettings s;
    s.beginGroup(kSettingsGroup);
    if (seq.isEmpty()) s.remove(key);
    else               s.setValue(key, seq.toString());
    s.endGroup();
    return true;
}

void KeybindingsManager::resetAll()
{
    QSettings s;
    s.beginGroup(kSettingsGroup);
    s.remove(QString());      // clears the whole group
    s.endGroup();
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        it->currentShortcut = it->defaultShortcut;
        if (it->action) it->action->setShortcut(it->defaultShortcut);
    }
}

void KeybindingsManager::writeAll() const
{
    QSettings s;
    s.beginGroup(kSettingsGroup);
    s.remove(QString());
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        if (it->currentShortcut != it->defaultShortcut)
            s.setValue(it.key(), it->currentShortcut.toString());
    }
    s.endGroup();
}
