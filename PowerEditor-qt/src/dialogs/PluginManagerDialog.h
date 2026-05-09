#pragma once

#include <QDialog>
#include <QPointer>

class PythonPluginHost;
class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QLabel;

// PluginManagerDialog — lists discovered Python plugins, lets the user reload
// individual files, and exposes the user plugin directory ("Open folder…").
// Read-only listing — installation is just dropping .py files into the dir.
class PluginManagerDialog : public QDialog {
    Q_OBJECT
public:
    PluginManagerDialog(PythonPluginHost* host, QWidget* parent = nullptr);

private slots:
    void rebuildList();
    void onReloadSelected();
    void onReloadAll();
    void onOpenFolder();

private:
    QPointer<PythonPluginHost> m_host;
    QTreeWidget* m_tree    = nullptr;
    QPushButton* m_reload  = nullptr;
    QPushButton* m_reloadAll = nullptr;
    QPushButton* m_openDir = nullptr;
    QLabel*      m_status  = nullptr;
};
