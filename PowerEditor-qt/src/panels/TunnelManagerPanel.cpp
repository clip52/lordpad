#include "TunnelManagerPanel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

TunnelManagerPanel::TunnelManagerPanel(QWidget* parent) : QDockWidget(tr("Túneis SSH"), parent)
{
    setObjectName(QStringLiteral("TunnelManagerPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    m_name = new QLineEdit(root); m_name->setPlaceholderText(tr("nome"));
    m_host = new QLineEdit(root); m_host->setPlaceholderText(tr("user@host"));
    m_local = new QLineEdit(root); m_local->setPlaceholderText(tr("local porta"));
    m_remote = new QLineEdit(root); m_remote->setPlaceholderText(tr("dest:porta"));
    m_addBtn = new QPushButton(tr("+ túnel"), root);
    m_delBtn = new QPushButton(tr("− túnel"), root);
    m_startBtn = new QPushButton(tr("Conectar"), root);
    m_stopBtn  = new QPushButton(tr("Desconectar"), root);
    m_tree = new QTreeWidget(root);
    m_tree->setHeaderLabels({ tr("Nome"), tr("Host"), tr("Local"), tr("Destino"), tr("Estado") });
    m_tree->setRootIsDecorated(false);
    m_status = new QLabel(root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(m_name); row1->addWidget(m_host);
    row1->addWidget(m_local); row1->addWidget(m_remote);
    row1->addWidget(m_addBtn); row1->addWidget(m_delBtn);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(m_startBtn);
    row2->addWidget(m_stopBtn);
    row2->addStretch(1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(m_tree, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_addBtn, &QPushButton::clicked, this, &TunnelManagerPanel::onAdd);
    connect(m_delBtn, &QPushButton::clicked, this, &TunnelManagerPanel::onRemove);
    connect(m_startBtn, &QPushButton::clicked, this, &TunnelManagerPanel::onStart);
    connect(m_stopBtn,  &QPushButton::clicked, this, &TunnelManagerPanel::onStop);

    load();
}

TunnelManagerPanel::~TunnelManagerPanel()
{
    for (auto* p : m_procs) { if (p) { p->kill(); p->waitForFinished(1000); p->deleteLater(); } }
}

QString TunnelManagerPanel::rowKey(int row) const
{
    auto* it = m_tree->topLevelItem(row);
    return it ? it->text(0) : QString();
}

void TunnelManagerPanel::onAdd()
{
    if (m_name->text().trimmed().isEmpty() || m_host->text().trimmed().isEmpty()) return;
    auto* it = new QTreeWidgetItem(m_tree);
    it->setText(0, m_name->text().trimmed());
    it->setText(1, m_host->text().trimmed());
    it->setText(2, m_local->text().trimmed());
    it->setText(3, m_remote->text().trimmed());
    it->setText(4, QStringLiteral("parado"));
    m_name->clear(); m_host->clear(); m_local->clear(); m_remote->clear();
    persist();
}

void TunnelManagerPanel::onRemove()
{
    auto* it = m_tree->currentItem();
    if (!it) return;
    const QString key = it->text(0);
    if (auto* p = m_procs.value(key, nullptr)) { p->kill(); p->deleteLater(); m_procs.remove(key); }
    delete m_tree->takeTopLevelItem(m_tree->indexOfTopLevelItem(it));
    persist();
}

void TunnelManagerPanel::onStart()
{
    auto* it = m_tree->currentItem();
    if (!it) return;
    const QString key = it->text(0);
    if (m_procs.contains(key)) { m_status->setText(tr("Já conectado.")); return; }
    const QString ssh = QStandardPaths::findExecutable(QStringLiteral("ssh"));
    if (ssh.isEmpty()) { m_status->setText(tr("ssh não encontrado.")); return; }
    auto* p = new QProcess(this);
    const QString l = it->text(2);
    const QString r = it->text(3);
    const QString h = it->text(1);
    p->start(ssh, { QStringLiteral("-N"), QStringLiteral("-L"),
                     QStringLiteral("%1:%2").arg(l, r),
                     QStringLiteral("-o"), QStringLiteral("ServerAliveInterval=30"),
                     QStringLiteral("-o"), QStringLiteral("ExitOnForwardFailure=yes"),
                     h });
    if (!p->waitForStarted(3000)) {
        m_status->setText(tr("Falha ao iniciar."));
        p->deleteLater(); return;
    }
    m_procs.insert(key, p);
    it->setText(4, QStringLiteral("ativo"));
    connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this, key](int, QProcess::ExitStatus) {
        m_procs.remove(key);
        for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
            auto* it = m_tree->topLevelItem(i);
            if (it->text(0) == key) it->setText(4, QStringLiteral("parado"));
        }
    });
    m_status->setText(tr("Conectado %1 → %2").arg(l, r));
}

void TunnelManagerPanel::onStop()
{
    auto* it = m_tree->currentItem();
    if (!it) return;
    const QString key = it->text(0);
    if (auto* p = m_procs.value(key, nullptr)) {
        p->kill();
        m_procs.remove(key);
        p->deleteLater();
        it->setText(4, QStringLiteral("parado"));
        m_status->setText(tr("Desconectado %1").arg(key));
    }
}

void TunnelManagerPanel::persist() const
{
    QSettings s; s.beginGroup(QStringLiteral("Tunnels"));
    s.remove(QString());
    QStringList names;
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto* it = m_tree->topLevelItem(i);
        const QString key = it->text(0);
        names << key;
        s.setValue(QStringLiteral("%1/host").arg(key), it->text(1));
        s.setValue(QStringLiteral("%1/local").arg(key), it->text(2));
        s.setValue(QStringLiteral("%1/remote").arg(key), it->text(3));
    }
    s.setValue(QStringLiteral("names"), names);
}

void TunnelManagerPanel::load()
{
    QSettings s; s.beginGroup(QStringLiteral("Tunnels"));
    for (const QString& key : s.value(QStringLiteral("names")).toStringList()) {
        auto* it = new QTreeWidgetItem(m_tree);
        it->setText(0, key);
        it->setText(1, s.value(QStringLiteral("%1/host").arg(key)).toString());
        it->setText(2, s.value(QStringLiteral("%1/local").arg(key)).toString());
        it->setText(3, s.value(QStringLiteral("%1/remote").arg(key)).toString());
        it->setText(4, QStringLiteral("parado"));
    }
}
