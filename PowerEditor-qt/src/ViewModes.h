#pragma once
#include <QObject>
#include <QList>
#include <QPointer>

class QMainWindow;
class QAction;
class QMenuBar;
class QStatusBar;
class QToolBar;
class QDockWidget;
class MultiView;
class EditorTab;

// ViewModes: shortcuts and modes that affect window layout / tab navigation.
//
//  - Ctrl+1..Ctrl+9 : jump to MultiView tab number 1..9 (across all groups).
//  - Ctrl+Tab / Ctrl+Shift+Tab : open MRU quick-switch dialog.
//  - F11 : toggle Zen mode (hides menubar, toolbars, statusbar, dock widgets).
//
// All state is per-session; nothing is persisted.
class ViewModes : public QObject {
    Q_OBJECT
public:
    ViewModes(QMainWindow* main, MultiView* multi, QObject* parent = nullptr);

    // Installs Ctrl+1..9, Ctrl+Tab, Ctrl+Shift+Tab and F11 as QShortcut on the main window.
    void registerShortcuts();

    // Returns a checkable QAction that toggles Zen mode (parented to `parent`).
    QAction* zenAction(QObject* parent);

public slots:
    void goToTab(int oneBasedIndex);     // 1-based; ignored if out of range
    void quickSwitchForward();           // Ctrl+Tab
    void quickSwitchBackward();          // Ctrl+Shift+Tab
    void toggleZenMode();                // F11

private slots:
    void onMultiViewCurrentChanged(EditorTab* tab);

private:
    void touchMru(EditorTab* tab);
    void rebuildMruFromMultiView();
    void openQuickSwitch(bool forward);

    QPointer<QMainWindow> m_main;
    QPointer<MultiView>   m_multiView;
    QPointer<QAction>     m_zenAction;

    // MRU: most-recent first.
    QList<QPointer<EditorTab>> m_mru;

    // Zen mode bookkeeping.
    struct ZenSnapshot {
        bool active = false;
        bool menuBarVisible = true;
        bool statusBarVisible = true;
        QList<QPointer<QToolBar>>     toolBars;
        QList<bool>                   toolBarsVisible;
        QList<QPointer<QDockWidget>>  docks;
        QList<bool>                   docksVisible;
    };
    ZenSnapshot m_zen;
};
