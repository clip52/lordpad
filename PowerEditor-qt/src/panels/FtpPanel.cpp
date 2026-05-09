#include "FtpPanel.h"

#include "../network/FtpClient.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

FtpPanel::FtpPanel(QWidget* parent)
    : QDockWidget(tr("FTP"), parent)
{
    setObjectName(QStringLiteral("FtpPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    auto* lay  = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->setSpacing(4);

    // Connect bar
    {
        auto* row = new QHBoxLayout();
        m_host = new QLineEdit(root); m_host->setPlaceholderText(tr("host"));
        m_port = new QSpinBox(root);  m_port->setRange(1, 65535); m_port->setValue(21);
        m_user = new QLineEdit(root); m_user->setPlaceholderText(tr("user"));
        m_pass = new QLineEdit(root); m_pass->setPlaceholderText(tr("senha")); m_pass->setEchoMode(QLineEdit::Password);
        m_btnConnect    = new QPushButton(tr("Conectar"), root);
        m_btnDisconnect = new QPushButton(tr("Desconectar"), root);
        m_btnDisconnect->setEnabled(false);

        row->addWidget(new QLabel(tr("Host"), root));
        row->addWidget(m_host, 2);
        row->addWidget(new QLabel(tr("Porta"), root));
        row->addWidget(m_port);
        row->addWidget(new QLabel(tr("User"), root));
        row->addWidget(m_user, 1);
        row->addWidget(new QLabel(tr("Senha"), root));
        row->addWidget(m_pass, 1);
        row->addWidget(m_btnConnect);
        row->addWidget(m_btnDisconnect);
        lay->addLayout(row);
    }

    m_pathLabel = new QLabel(tr("(desconectado)"), root);
    m_pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    lay->addWidget(m_pathLabel);

    m_tree = new QTreeWidget(root);
    m_tree->setHeaderLabels({ tr("Nome"), tr("Tamanho") });
    m_tree->setRootIsDecorated(false);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    lay->addWidget(m_tree, 1);

    {
        auto* row = new QHBoxLayout();
        m_btnRefresh    = new QPushButton(tr("Atualizar"), root);
        m_btnCdUp       = new QPushButton(tr(".."), root);
        m_btnUpload     = new QPushButton(tr("Upload…"), root);
        m_btnDownload   = new QPushButton(tr("Download…"), root);
        m_btnOpenRemote = new QPushButton(tr("Abrir remoto"), root);
        m_btnDelete     = new QPushButton(tr("Apagar"), root);
        for (auto* b : { m_btnRefresh, m_btnCdUp, m_btnUpload, m_btnDownload, m_btnOpenRemote, m_btnDelete })
            b->setEnabled(false);
        row->addWidget(m_btnCdUp);
        row->addWidget(m_btnRefresh);
        row->addStretch(1);
        row->addWidget(m_btnUpload);
        row->addWidget(m_btnDownload);
        row->addWidget(m_btnOpenRemote);
        row->addWidget(m_btnDelete);
        lay->addLayout(row);
    }

    m_progress = new QProgressBar(root);
    m_progress->setRange(0, 100);
    m_progress->hide();
    lay->addWidget(m_progress);

    setWidget(root);

    m_client = new FtpClient(this);
    connect(m_client, &FtpClient::transferProgress, this,
            [this](qint64 done, qint64 total) {
                if (total <= 0) {
                    m_progress->setRange(0, 0);   // indeterminate
                } else {
                    m_progress->setRange(0, 100);
                    m_progress->setValue(static_cast<int>(done * 100 / total));
                }
                m_progress->show();
            });
    connect(m_client, &FtpClient::serverMessage, this, &FtpPanel::statusMessage);

    connect(m_btnConnect,    &QPushButton::clicked, this, &FtpPanel::onConnect);
    connect(m_btnDisconnect, &QPushButton::clicked, this, &FtpPanel::onDisconnect);
    connect(m_btnRefresh,    &QPushButton::clicked, this, &FtpPanel::onRefresh);
    connect(m_btnCdUp,       &QPushButton::clicked, this, &FtpPanel::onCdUp);
    connect(m_btnUpload,     &QPushButton::clicked, this, &FtpPanel::onUpload);
    connect(m_btnDownload,   &QPushButton::clicked, this, &FtpPanel::onDownload);
    connect(m_btnOpenRemote, &QPushButton::clicked, this, &FtpPanel::onOpenRemote);
    connect(m_btnDelete,     &QPushButton::clicked, this, &FtpPanel::onDelete);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, &FtpPanel::onItemDoubleClicked);
}

FtpPanel::~FtpPanel() = default;

QString FtpPanel::cacheRoot() const {
    QDir d(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/ftp");
    if (!d.exists()) d.mkpath(".");
    return d.absolutePath();
}

QString FtpPanel::cachePathFor(const QString& host, const QString& remote) const {
    QDir d(cacheRoot() + "/" + host + remote);
    QDir().mkpath(d.absoluteFilePath(".").section('/', 0, -2));
    return d.absoluteFilePath(".").section('/', 0, -2);
}

QString FtpPanel::remotePathFor(const QString& name) const {
    QString cur = m_client ? m_client->currentDir() : QString();
    if (cur.isEmpty() || cur == "/") return "/" + name;
    if (cur.endsWith('/')) return cur + name;
    return cur + "/" + name;
}

void FtpPanel::onConnect()
{
    if (!m_client) return;
    QString err;
    if (!m_client->connectAndLogin(m_host->text(), static_cast<quint16>(m_port->value()),
                                   m_user->text(), m_pass->text(), &err)) {
        QMessageBox::warning(this, tr("FTP"), err);
        return;
    }
    m_currentHost = m_host->text();
    for (auto* b : { m_btnDisconnect, m_btnRefresh, m_btnCdUp, m_btnUpload,
                     m_btnDownload, m_btnOpenRemote, m_btnDelete })
        b->setEnabled(true);
    m_btnConnect->setEnabled(false);
    rebuildTree();
}

void FtpPanel::onDisconnect()
{
    if (!m_client) return;
    m_client->disconnectFromHost();
    m_tree->clear();
    m_pathLabel->setText(tr("(desconectado)"));
    for (auto* b : { m_btnDisconnect, m_btnRefresh, m_btnCdUp, m_btnUpload,
                     m_btnDownload, m_btnOpenRemote, m_btnDelete })
        b->setEnabled(false);
    m_btnConnect->setEnabled(true);
}

void FtpPanel::onRefresh() { rebuildTree(); }

void FtpPanel::rebuildTree()
{
    if (!m_client || !m_client->isConnected()) return;
    QList<FtpEntry> entries;
    QString err;
    if (!m_client->listDir(QString(), entries, &err)) {
        QMessageBox::warning(this, tr("FTP"), err);
        return;
    }
    m_tree->clear();
    for (const FtpEntry& e : entries) {
        auto* it = new QTreeWidgetItem(m_tree);
        it->setText(0, (e.isDir ? QStringLiteral("📁 ") : QStringLiteral("   ")) + e.name);
        it->setText(1, e.isDir ? QString() : QString::number(e.size));
        it->setData(0, Qt::UserRole, e.name);
        it->setData(0, Qt::UserRole + 1, e.isDir);
    }
    m_pathLabel->setText(tr("Remote: %1").arg(m_client->currentDir()));
    m_progress->hide();
}

void FtpPanel::onItemDoubleClicked(QTreeWidgetItem* item, int)
{
    if (!item || !m_client) return;
    const QString name = item->data(0, Qt::UserRole).toString();
    const bool isDir   = item->data(0, Qt::UserRole + 1).toBool();
    if (isDir) {
        QString err;
        if (!m_client->cwd(name, &err)) {
            QMessageBox::warning(this, tr("FTP"), err);
            return;
        }
        rebuildTree();
    } else {
        onOpenRemote();
    }
}

QString FtpPanel::currentRemoteSelection() const {
    auto items = m_tree->selectedItems();
    if (items.isEmpty()) return {};
    return items.first()->data(0, Qt::UserRole).toString();
}

void FtpPanel::onUpload()
{
    if (!m_client) return;
    const QString src = QFileDialog::getOpenFileName(this, tr("Upload via FTP"));
    if (src.isEmpty()) return;
    const QString remote = remotePathFor(QFileInfo(src).fileName());
    QString err;
    if (!m_client->store(src, remote, &err)) {
        QMessageBox::warning(this, tr("FTP"), err);
    }
    m_progress->hide();
    rebuildTree();
}

void FtpPanel::onDownload()
{
    if (!m_client) return;
    const QString name = currentRemoteSelection();
    if (name.isEmpty()) return;
    const QString dest = QFileDialog::getSaveFileName(this, tr("Download de %1").arg(name), name);
    if (dest.isEmpty()) return;
    QString err;
    const QString remote = remotePathFor(name);
    if (!m_client->retrieve(remote, dest, &err)) {
        QMessageBox::warning(this, tr("FTP"), err);
    }
    m_progress->hide();
}

void FtpPanel::onOpenRemote()
{
    if (!m_client) return;
    const QString name = currentRemoteSelection();
    if (name.isEmpty()) return;
    const QString remote = remotePathFor(name);

    QDir cacheDir(cacheRoot() + "/" + m_currentHost
                  + (remote.startsWith('/') ? QString() : QStringLiteral("/"))
                  + QFileInfo(remote).path());
    cacheDir.mkpath(".");
    const QString local = cacheDir.absoluteFilePath(QFileInfo(remote).fileName());

    QString err;
    if (!m_client->retrieve(remote, local, &err)) {
        QMessageBox::warning(this, tr("FTP"), err);
        return;
    }
    m_localToRemote.insert(local, remote);
    m_progress->hide();
    emit openLocalFile(local);
}

void FtpPanel::onDelete()
{
    if (!m_client) return;
    const QString name = currentRemoteSelection();
    if (name.isEmpty()) return;
    if (QMessageBox::question(this, tr("FTP"),
            tr("Apagar '%1' no servidor?").arg(name)) != QMessageBox::Yes) return;
    QString err;
    if (!m_client->deleteFile(remotePathFor(name), &err)) {
        QMessageBox::warning(this, tr("FTP"), err);
    }
    rebuildTree();
}

void FtpPanel::onCdUp()
{
    if (!m_client) return;
    QString err;
    if (!m_client->cwd("..", &err)) {
        QMessageBox::warning(this, tr("FTP"), err);
        return;
    }
    rebuildTree();
}

bool FtpPanel::isRemoteCachedFile(const QString& localPath, QString* outRemotePath) const
{
    auto it = m_localToRemote.constFind(localPath);
    if (it == m_localToRemote.constEnd()) return false;
    if (outRemotePath) *outRemotePath = it.value();
    return true;
}

void FtpPanel::commitRemoteSave(const QString& localPath)
{
    if (!m_client || !m_client->isConnected()) return;
    auto it = m_localToRemote.constFind(localPath);
    if (it == m_localToRemote.constEnd()) return;
    QString err;
    if (!m_client->store(localPath, it.value(), &err)) {
        QMessageBox::warning(this, tr("FTP — re-upload"), err);
        return;
    }
    emit statusMessage(tr("FTP: '%1' atualizado.").arg(it.value()));
    m_progress->hide();
}
