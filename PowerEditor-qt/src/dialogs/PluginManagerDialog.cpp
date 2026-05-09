#include "PluginManagerDialog.h"

#include "../plugins/PythonPluginHost.h"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QUrl>
#include <QVBoxLayout>

PluginManagerDialog::PluginManagerDialog(PythonPluginHost* host, QWidget* parent)
    : QDialog(parent), m_host(host)
{
    setWindowTitle(tr("Plugins (Python)"));
    resize(640, 380);

    m_status = new QLabel(this);
    m_tree = new QTreeWidget(this);
    m_tree->setRootIsDecorated(false);
    m_tree->setHeaderLabels({ tr("Plugin"), tr("Estado"), tr("Detalhes") });
    m_tree->header()->setStretchLastSection(true);
    m_tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);

    m_reload    = new QPushButton(tr("Recarregar selecionado"), this);
    m_reloadAll = new QPushButton(tr("Recarregar todos"), this);
    m_openDir   = new QPushButton(tr("Abrir pasta…"), this);

    auto* row = new QHBoxLayout();
    row->addWidget(m_reload);
    row->addWidget(m_reloadAll);
    row->addStretch(1);
    row->addWidget(m_openDir);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::accept);

    auto* lay = new QVBoxLayout(this);
    lay->addWidget(m_status);
    lay->addWidget(m_tree, /*stretch*/1);
    lay->addLayout(row);
    lay->addWidget(bb);

    connect(m_reload,    &QPushButton::clicked, this, &PluginManagerDialog::onReloadSelected);
    connect(m_reloadAll, &QPushButton::clicked, this, &PluginManagerDialog::onReloadAll);
    connect(m_openDir,   &QPushButton::clicked, this, &PluginManagerDialog::onOpenFolder);
    if (m_host) {
        connect(m_host.data(), &PythonPluginHost::pluginsChanged,
                this, &PluginManagerDialog::rebuildList);
    }

    rebuildList();
}

void PluginManagerDialog::rebuildList()
{
    m_tree->clear();
    if (!m_host) {
        m_status->setText(tr("Subsistema de plugins indisponível."));
        return;
    }
    if (!m_host->isAvailable()) {
        m_status->setText(tr("Este build não foi compilado com python3-embed. "
                             "Os plugins são listados, mas não podem ser carregados."));
    } else {
        m_status->setText(tr("Plugins em: %1").arg(m_host->pluginDir()));
    }

    const QStringList files = m_host->discoverPlugins();
    const QStringList loaded = m_host->loadedPlugins();
    for (const QString& path : files) {
        const QFileInfo fi(path);
        const bool isLoaded = loaded.contains(path);
        const QString err = m_host->errorFor(path);

        QString state;
        if (!err.isEmpty())  state = tr("Erro");
        else if (isLoaded)   state = tr("Ativo");
        else                 state = tr("(não carregado)");

        auto* it = new QTreeWidgetItem(m_tree);
        it->setText(0, fi.fileName());
        it->setText(1, state);
        it->setText(2, err.isEmpty() ? path : err);
        it->setData(0, Qt::UserRole, path);
        it->setToolTip(2, err.isEmpty() ? path : err);
    }
    m_tree->resizeColumnToContents(0);
    m_tree->resizeColumnToContents(1);
}

void PluginManagerDialog::onReloadSelected()
{
    if (!m_host) return;
    auto items = m_tree->selectedItems();
    if (items.isEmpty()) return;
    const QString path = items.first()->data(0, Qt::UserRole).toString();
    if (path.isEmpty()) return;
    QString err;
    m_host->loadPlugin(path, &err);
    rebuildList();
}

void PluginManagerDialog::onReloadAll()
{
    if (!m_host) return;
    m_host->reloadAll();
    rebuildList();
}

void PluginManagerDialog::onOpenFolder()
{
    if (!m_host) return;
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_host->pluginDir()));
}
