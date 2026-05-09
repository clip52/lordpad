#include "SshfsPanel.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QRegularExpression>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>
#include <QTreeView>
#include <QVBoxLayout>
#include <QWidget>

SshfsPanel::SshfsPanel(QWidget* parent) : QDockWidget(tr("SSHFS"), parent)
{
    setObjectName(QStringLiteral("SshfsPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    m_target = new QLineEdit(root);
    m_target->setPlaceholderText(QStringLiteral("user@host:/remote/path"));
    m_mountpoint = new QLineEdit(root);
    m_mountpoint->setPlaceholderText(QStringLiteral("ponto local de mount (vazio = auto)"));
    m_pickBtn  = new QPushButton(tr("…"), root);
    m_mountBtn = new QPushButton(tr("Montar"), root);
    m_umountBtn = new QPushButton(tr("Desmontar"), root);
    m_mounts = new QListWidget(root); m_mounts->setMaximumWidth(280);

    m_fsModel = new QFileSystemModel(this);
    m_browser = new QTreeView(root);
    m_browser->setModel(m_fsModel);
    m_browser->setAnimated(false);
    m_browser->setIndentation(16);
    m_browser->setSortingEnabled(true);
    for (int c = 1; c < m_fsModel->columnCount(); ++c) m_browser->setColumnHidden(c, true);
    m_status = new QLabel(root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("Remote:"), root));
    row1->addWidget(m_target, 2);
    row1->addWidget(new QLabel(tr("→"), root));
    row1->addWidget(m_mountpoint, 1);
    row1->addWidget(m_pickBtn);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(m_mountBtn);
    row2->addWidget(m_umountBtn);
    row2->addStretch(1);

    auto* split = new QSplitter(Qt::Horizontal, root);
    split->addWidget(m_mounts); split->addWidget(m_browser);
    split->setStretchFactor(0, 0); split->setStretchFactor(1, 1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(split, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_pickBtn, &QPushButton::clicked, this, &SshfsPanel::onPickMountpoint);
    connect(m_mountBtn, &QPushButton::clicked, this, &SshfsPanel::onMount);
    connect(m_umountBtn, &QPushButton::clicked, this, &SshfsPanel::onUnmount);
    connect(m_mounts, &QListWidget::itemActivated, this, &SshfsPanel::onMountSelected);
    connect(m_mounts, &QListWidget::currentItemChanged, this,
            [this](QListWidgetItem* cur, QListWidgetItem*) { Q_UNUSED(cur); onMountSelected(); });
    connect(m_browser, &QTreeView::doubleClicked, this, &SshfsPanel::onFileActivated);

    if (QStandardPaths::findExecutable(QStringLiteral("sshfs")).isEmpty())
        m_status->setText(tr("sshfs não encontrado — instale fuse-sshfs."));
    load();
}

SshfsPanel::~SshfsPanel() = default;

QString SshfsPanel::defaultMountpoint(const QString& target) const
{
    QString safe = target;
    safe.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]")), QStringLiteral("_"));
    const QString home = QDir::homePath();
    return home + QStringLiteral("/.cache/notepadpp-qt/sshfs/") + safe;
}

void SshfsPanel::onPickMountpoint()
{
    const QString p = QFileDialog::getExistingDirectory(this, tr("Mountpoint local"),
                                                          m_mountpoint->text());
    if (!p.isEmpty()) m_mountpoint->setText(p);
}

void SshfsPanel::onMount()
{
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("sshfs"));
    if (tool.isEmpty()) { m_status->setText(tr("sshfs não encontrado.")); return; }
    const QString target = m_target->text().trimmed();
    if (target.isEmpty()) { m_status->setText(tr("Target obrigatório.")); return; }

    QString mp = m_mountpoint->text().trimmed();
    if (mp.isEmpty()) mp = defaultMountpoint(target);
    QDir().mkpath(mp);

    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    // -o reconnect e ServerAliveInterval p/ resiliência; idmap=user pra evitar
    // surpresas de UID. Usuário precisa ter chave SSH (ou agente).
    p.start(tool, { target, mp,
                     QStringLiteral("-o"), QStringLiteral("reconnect"),
                     QStringLiteral("-o"), QStringLiteral("ServerAliveInterval=15"),
                     QStringLiteral("-o"), QStringLiteral("ServerAliveCountMax=3"),
                     QStringLiteral("-o"), QStringLiteral("idmap=user") });
    if (!p.waitForFinished(20000)) { p.kill(); m_status->setText(tr("Timeout sshfs.")); return; }
    if (p.exitCode() != 0) {
        m_status->setText(tr("Falhou: %1").arg(QString::fromUtf8(p.readAllStandardOutput()).trimmed()));
        return;
    }

    auto* it = new QListWidgetItem(QStringLiteral("%1 → %2").arg(target, mp), m_mounts);
    it->setData(Qt::UserRole, mp);
    it->setData(Qt::UserRole + 1, target);
    m_mounts->setCurrentItem(it);
    onMountSelected();
    m_status->setText(tr("Montado em %1").arg(mp));
    persist();
}

void SshfsPanel::onUnmount()
{
    auto* it = m_mounts->currentItem();
    if (!it) return;
    const QString mp = it->data(Qt::UserRole).toString();
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("fusermount"));
    if (!tool.isEmpty()) {
        QProcess p;
        p.start(tool, { QStringLiteral("-u"), mp });
        p.waitForFinished(8000);
    } else {
        // fallback: umount (pode requerer root)
        QProcess p;
        p.start(QStringLiteral("umount"), { mp });
        p.waitForFinished(8000);
    }
    delete m_mounts->takeItem(m_mounts->row(it));
    m_browser->setRootIndex(QModelIndex());
    persist();
    m_status->setText(tr("Desmontado %1").arg(mp));
}

void SshfsPanel::onMountSelected()
{
    auto* it = m_mounts->currentItem();
    if (!it) return;
    const QString mp = it->data(Qt::UserRole).toString();
    m_fsModel->setRootPath(mp);
    m_browser->setRootIndex(m_fsModel->index(mp));
}

void SshfsPanel::onFileActivated(const QModelIndex& idx)
{
    if (!idx.isValid()) return;
    if (m_fsModel->isDir(idx)) return;
    const QString p = m_fsModel->filePath(idx);
    emit openFileRequested(p);
}

void SshfsPanel::persist() const
{
    QStringList entries;
    for (int i = 0; i < m_mounts->count(); ++i) {
        auto* it = m_mounts->item(i);
        entries << it->data(Qt::UserRole + 1).toString() + QChar(0x1F)
                + it->data(Qt::UserRole).toString();
    }
    QSettings().setValue(QStringLiteral("Sshfs/active"), entries);
}

void SshfsPanel::load()
{
    QSettings s;
    for (const QString& e : s.value(QStringLiteral("Sshfs/active")).toStringList()) {
        const QString target = e.section(QChar(0x1F), 0, 0);
        const QString mp     = e.section(QChar(0x1F), 1);
        if (mp.isEmpty()) continue;
        // Verifica se o mount ainda está ativo lendo /proc/mounts.
        QFile mounts(QStringLiteral("/proc/mounts"));
        bool active = false;
        if (mounts.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString all = QString::fromUtf8(mounts.readAll());
            active = all.contains(QStringLiteral(" %1 ").arg(mp));
        }
        if (active) {
            auto* it = new QListWidgetItem(QStringLiteral("%1 → %2").arg(target, mp), m_mounts);
            it->setData(Qt::UserRole, mp);
            it->setData(Qt::UserRole + 1, target);
        }
    }
}
