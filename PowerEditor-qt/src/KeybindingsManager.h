#pragma once

#include <QObject>
#include <QString>
#include <QHash>
#include <QKeySequence>
#include <QPointer>
#include <QList>

class QAction;
class QWidget;

// KeybindingsManager — collects every QAction reachable from the main
// window, lets the user override each shortcut, and persists those
// overrides in QSettings under the "Keybindings" group.
//
// Action identity: we key by `text()` with mnemonic ampersands stripped.
// That's stable enough across runs for the actions this app creates from
// its `mk* lambdas` — none of them set objectName explicitly. If the menu
// labels are translated mid-life, the override key flips to the new label;
// users would just need to re-bind, which is the expected behaviour.
class KeybindingsManager : public QObject {
    Q_OBJECT
public:
    struct Entry {
        QString  key;                // mnemonic-stripped action text
        QPointer<QAction> action;    // weak pointer; the QAction belongs to MainWindow
        QKeySequence defaultShortcut;
        QKeySequence currentShortcut;
    };

    explicit KeybindingsManager(QObject* parent = nullptr);

    // Walk `host`'s QAction tree (menus + toolbars + child widgets) and
    // populate the entry table. Records each action's *current* shortcut as
    // the default — call this once before applyPersistedOverrides() so
    // "Reset to default" is meaningful afterwards.
    void collectFromHost(QWidget* host);

    // Apply the persisted QSettings overrides on top of the collected
    // defaults. Safe to call before any user interaction.
    void applyPersistedOverrides();

    // The full table — used by the dialog.
    QList<Entry> entries() const;

    // Update + persist a single binding. Empty `seq` clears the override
    // (action falls back to its default). Returns true if the entry exists.
    bool setOverride(const QString& key, const QKeySequence& seq);

    // Remove every override and reset all collected actions to their defaults.
    void resetAll();

private:
    static QString keyForAction(QAction* a);
    void writeAll() const;

    QHash<QString, Entry> m_entries;
};
