#include "ViewModes.h"

#include "MultiView.h"
#include "EditorTab.h"

#include <QAction>
#include <QDialog>
#include <QDockWidget>
#include <QKeyEvent>
#include <QListWidget>
#include <QMainWindow>
#include <QMenuBar>
#include <QShortcut>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QVBoxLayout>

namespace {

// Best-effort title for an EditorTab: prefer tabTitle(), fall back to displayPath().
QString labelForTab(EditorTab* tab)
{
    if (!tab) return QString();
    QString t = tab->tabTitle();
    if (t.isEmpty()) t = tab->displayPath();
    if (t.isEmpty()) t = ViewModes::tr("(sem título)");
    return t;
}

} // namespace

ViewModes::ViewModes(QMainWindow* main, MultiView* multi, QObject* parent)
    : QObject(parent)
    , m_main(main)
    , m_multiView(multi)
{
    if (m_multiView) {
        connect(m_multiView, &MultiView::currentTabChanged,
                this, &ViewModes::onMultiViewCurrentChanged);
        rebuildMruFromMultiView();
    }
}

void ViewModes::registerShortcuts()
{
    if (!m_main) return;

    // Ctrl+1 .. Ctrl+9 : jump to tab N
    for (int i = 1; i <= 9; ++i) {
        const QKeySequence seq(Qt::CTRL | static_cast<Qt::Key>(Qt::Key_0 + i));
        auto* sc = new QShortcut(seq, m_main);
        sc->setContext(Qt::WindowShortcut);
        connect(sc, &QShortcut::activated, this, [this, i]() { goToTab(i); });
    }

    // Ctrl+Tab / Ctrl+Shift+Tab : MRU quick switch
    auto* scNext = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab), m_main);
    scNext->setContext(Qt::WindowShortcut);
    connect(scNext, &QShortcut::activated, this, &ViewModes::quickSwitchForward);

    auto* scPrev = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab), m_main);
    scPrev->setContext(Qt::WindowShortcut);
    connect(scPrev, &QShortcut::activated, this, &ViewModes::quickSwitchBackward);

    // F11 : Zen mode
    auto* scZen = new QShortcut(QKeySequence(Qt::Key_F11), m_main);
    scZen->setContext(Qt::WindowShortcut);
    connect(scZen, &QShortcut::activated, this, &ViewModes::toggleZenMode);
}

QAction* ViewModes::zenAction(QObject* parent)
{
    if (m_zenAction) return m_zenAction;
    auto* act = new QAction(tr("Modo &Zen"), parent);
    act->setCheckable(true);
    act->setShortcut(QKeySequence(Qt::Key_F11));
    act->setStatusTip(tr("Esconde menus, barras e painéis para foco máximo"));
    connect(act, &QAction::triggered, this, &ViewModes::toggleZenMode);
    m_zenAction = act;
    return act;
}

void ViewModes::goToTab(int oneBasedIndex)
{
    if (!m_multiView) return;
    const int globalIdx = oneBasedIndex - 1;
    if (globalIdx < 0 || globalIdx >= m_multiView->tabCount()) return;

    EditorTab* tab = m_multiView->tabAt(globalIdx);
    if (!tab) return;

    auto loc = m_multiView->locateTab(tab);
    if (loc.first && loc.second >= 0) {
        loc.first->setCurrentIndex(loc.second);
        tab->setFocus();
    }
}

void ViewModes::quickSwitchForward()
{
    openQuickSwitch(true);
}

void ViewModes::quickSwitchBackward()
{
    openQuickSwitch(false);
}

void ViewModes::onMultiViewCurrentChanged(EditorTab* tab)
{
    if (tab) touchMru(tab);
}

void ViewModes::touchMru(EditorTab* tab)
{
    // Drop any expired weak refs and any matching pointer, then push to front.
    for (int i = m_mru.size() - 1; i >= 0; --i) {
        if (!m_mru[i] || m_mru[i].data() == tab)
            m_mru.removeAt(i);
    }
    m_mru.prepend(QPointer<EditorTab>(tab));
}

void ViewModes::rebuildMruFromMultiView()
{
    m_mru.clear();
    if (!m_multiView) return;
    EditorTab* current = m_multiView->currentTab();
    const int n = m_multiView->tabCount();
    if (current) m_mru.append(QPointer<EditorTab>(current));
    for (int i = 0; i < n; ++i) {
        EditorTab* t = m_multiView->tabAt(i);
        if (t && t != current)
            m_mru.append(QPointer<EditorTab>(t));
    }
}

void ViewModes::openQuickSwitch(bool forward)
{
    if (!m_main || !m_multiView) return;
    if (m_multiView->tabCount() < 2) return;

    // Build a clean MRU list backed by current tabs in MultiView.
    QList<EditorTab*> ordered;
    for (const auto& p : m_mru) {
        if (p && m_multiView->locateTab(p.data()).first)
            ordered.append(p.data());
    }
    // Append any tabs not yet seen by the MRU (preserves discovery order).
    const int n = m_multiView->tabCount();
    for (int i = 0; i < n; ++i) {
        EditorTab* t = m_multiView->tabAt(i);
        if (t && !ordered.contains(t))
            ordered.append(t);
    }
    if (ordered.size() < 2) return;

    QDialog dlg(m_main, Qt::Popup | Qt::FramelessWindowHint);
    dlg.setWindowModality(Qt::ApplicationModal);
    dlg.setObjectName(QStringLiteral("ViewModesQuickSwitch"));

    auto* layout = new QVBoxLayout(&dlg);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(0);

    auto* list = new QListWidget(&dlg);
    list->setUniformItemSizes(true);
    list->setFocusPolicy(Qt::StrongFocus);
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(list);

    for (EditorTab* t : ordered) {
        auto* item = new QListWidgetItem(labelForTab(t), list);
        item->setToolTip(t->displayPath());
    }

    // Initial selection: second item if going forward (i.e. previous tab),
    // last item if going backward.
    const int initial = forward ? 1 : (list->count() - 1);
    list->setCurrentRow(initial);

    // Sizing: snug to content, but capped.
    const int rowH = list->sizeHintForRow(0);
    const int rows = qMin(list->count(), 12);
    int h = (rowH > 0 ? rowH : 22) * rows + 16;
    int w = 420;
    dlg.resize(w, h);
    if (m_main) {
        const QPoint center = m_main->geometry().center();
        dlg.move(center.x() - w / 2, center.y() - h / 2);
    }

    // Ctrl+Tab / Ctrl+Shift+Tab cycle the selection while open.
    auto cycle = [list](int delta) {
        const int n = list->count();
        if (n <= 0) return;
        int row = list->currentRow() + delta;
        if (row < 0) row = n - 1;
        if (row >= n) row = 0;
        list->setCurrentRow(row);
    };

    auto* scFwd = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Tab), &dlg);
    scFwd->setContext(Qt::WindowShortcut);
    QObject::connect(scFwd, &QShortcut::activated, &dlg, [cycle]() { cycle(+1); });

    auto* scBwd = new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab), &dlg);
    scBwd->setContext(Qt::WindowShortcut);
    QObject::connect(scBwd, &QShortcut::activated, &dlg, [cycle]() { cycle(-1); });

    QObject::connect(list, &QListWidget::itemActivated,
                     &dlg, [&dlg]() { dlg.accept(); });

    // Enter/Return inside the list also accepts.
    list->installEventFilter(&dlg);
    QObject::connect(list, &QListWidget::itemClicked,
                     &dlg, [&dlg]() { dlg.accept(); });

    if (dlg.exec() == QDialog::Accepted) {
        const int row = list->currentRow();
        if (row >= 0 && row < ordered.size()) {
            EditorTab* chosen = ordered[row];
            auto loc = m_multiView->locateTab(chosen);
            if (loc.first && loc.second >= 0) {
                loc.first->setCurrentIndex(loc.second);
                chosen->setFocus();
            }
        }
    }
}

void ViewModes::toggleZenMode()
{
    if (!m_main) return;

    if (!m_zen.active) {
        // ----- Enter Zen -----
        m_zen.toolBars.clear();
        m_zen.toolBarsVisible.clear();
        m_zen.docks.clear();
        m_zen.docksVisible.clear();

        if (auto* mb = m_main->menuBar()) {
            m_zen.menuBarVisible = mb->isVisible();
            mb->setVisible(false);
        }
        if (auto* sb = m_main->statusBar()) {
            m_zen.statusBarVisible = sb->isVisible();
            sb->setVisible(false);
        }

        const auto toolbars = m_main->findChildren<QToolBar*>();
        for (QToolBar* tb : toolbars) {
            m_zen.toolBars.append(QPointer<QToolBar>(tb));
            m_zen.toolBarsVisible.append(tb->isVisible());
            tb->setVisible(false);
        }

        const auto docks = m_main->findChildren<QDockWidget*>();
        for (QDockWidget* d : docks) {
            m_zen.docks.append(QPointer<QDockWidget>(d));
            m_zen.docksVisible.append(d->isVisible());
            d->setVisible(false);
        }

        m_zen.active = true;
        if (m_zenAction) m_zenAction->setChecked(true);
    } else {
        // ----- Exit Zen -----
        if (auto* mb = m_main->menuBar())
            mb->setVisible(m_zen.menuBarVisible);
        if (auto* sb = m_main->statusBar())
            sb->setVisible(m_zen.statusBarVisible);

        for (int i = 0; i < m_zen.toolBars.size(); ++i) {
            if (auto* tb = m_zen.toolBars[i].data())
                tb->setVisible(m_zen.toolBarsVisible.value(i, true));
        }
        for (int i = 0; i < m_zen.docks.size(); ++i) {
            if (auto* d = m_zen.docks[i].data())
                d->setVisible(m_zen.docksVisible.value(i, true));
        }

        m_zen.toolBars.clear();
        m_zen.toolBarsVisible.clear();
        m_zen.docks.clear();
        m_zen.docksVisible.clear();
        m_zen.active = false;
        if (m_zenAction) m_zenAction->setChecked(false);
    }
}
