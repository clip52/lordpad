#include "SftpPanel.h"

#include "../network/SftpClient.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

SftpPanel::SftpPanel(QWidget* parent) : QDockWidget(tr("SFTP"), parent)
{
    setObjectName(QStringLiteral("SftpPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    auto* lay  = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->setSpacing(4);

    {
        auto* row = new QHBoxLayout();
        m_host = new QLineEdit(root); m_host->setPlaceholderText(tr("host"));
        m_user = new QLineEdit(root);
        m_port = new QSpinBox(root);  m_port->setRange(1, 65535); m_port->setValue(22);
        m_id   = new QLineEdit(root); m_id->setPlaceholderText(tr("(opcional) chave"));
        m_btnId = new QPushButton(QStringLiteral("…"), root); m_btnId->setMaximumWidth(32);
        m_btnConnect    = new QPushButton(tr("Conectar"), root);
        m_btnDisconnect = new QPushButton(tr("Desconectar"), root);
        m_btnDisconnect->setEnabled(false);

        row->addWidget(new QLabel(tr("Host"), root));
        row->addWidget(m_host, 2);
        row->addWidget(new QLabel(tr("User"), root));
        row->addWidget(m_user, 1);
        row->addWidget(new QLabel(tr("Porta"), root));
        row->addWidget(m_port);
        row->addWidget(new QLabel(tr("Chave"), root));
        row->addWidget(m_id, 1);
        row->addWidget(m_btnId);
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
        m_btnCdUp       = new QPushButton(QStringLiteral(".."), root);
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

    setWidget(root);

    m_client = new SftpClient(this);

    connect(m_btnId,         &QPushButton::clicked, this, &SftpPanel::onPickIdentity);
    connect(m_btnConnect,    &QPushButton::clicked, this, &SftpPanel::onConnect);
    connect(m_btnDisconnect, &QPushButton::clicked, this, &SftpPanel::onDisconnect);
    connect(m_btnRefresh,    &QPushButton::clicked, this, &SftpPanel::onRefresh);
    connect(m_btnCdUp,       &QPushButton::clicked, this, &SftpPanel::onCdUp);
    connect(m_btnUpload,     &QPushButton::clicked, this, &SftpPanel::onUpload);
    connect(m_btnDownload,   &QPushButton::clicked, this, &SftpPanel::onDownload);
    connect(m_btnOpenRemote, &QPushButton::clicked, this, &SftpPanel::onOpenRemote);
    connect(m_btnDelete,     &QPushButton::clicked, this, &SftpPanel::onDelete);
    connect(m_tree, &QTreeWidget::itemDoubleClicked, this, &SftpPanel::onItemDoubleClicked);
}

SftpPanel::~SftpPanel() = default;

QString SftpPanel::cacheRoot() const {
    QDir d(QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/sftp");
    if (!d.exists()) d.mkpath(".");
    return d.absolutePath();
}

QString SftpPanel::cacheLocalFor(const QString& remoteAbsolute) const
{
    QString p = cacheRoot() + "/" + m_currentHost + remoteAbsolute;
    QFileInfo fi(p);
    QDir().mkpath(fi.absolutePath());
    return fi.absoluteFilePath();
}

void SftpPanel::onPickIdentity()
{
    const QString p = QFileDialog::getOpenFileName(this, tr("Chave SSH"),
        QDir::homePath() + "/.ssh");
    if (!p.isEmpty()) m_id->setText(p);
}

void SftpPanel::onConnect()
{
    if (!m_client) return;
    m_client->setProfile(m_host->text(), m_user->text(),
                         static_cast<quint16>(m_port->value()), m_id->text());
    m_currentHost = m_host->text();
    QString err;
    if (!m_client->ping(&err)) {
        QMessageBox::warning(this, tr("SFTP"),
            tr("Falha ao conectar:\n%1\n\nDica: a autenticação é por chave (BatchMode=yes). "
               "Verifique sua chave/ssh-agent.").arg(err));
        return;
    }
    // Start at the user's $HOME on the remote — `pwd` returns it.
    QByteArray out, dummy;
    QStringList args = m_client->commonSshArgs();
    args << m_client->sshTarget() << QStringLiteral("pwd");
    QProcess pwd;
    pwd.start(QStringLiteral("ssh"), args);
    if (pwd.waitForStarted(2000) && pwd.waitForFinished(8000) && pwd.exitCode() == 0) {
        QString remoteHome = QString::fromUtf8(pwd.readAllStandardOutput()).trimmed();
        if (!remoteHome.isEmpty()) m_client->setCurrentDir(remoteHome);
    }

    for (auto* b : { m_btnDisconnect, m_btnRefresh, m_btnCdUp, m_btnUpload,
                     m_btnDownload, m_btnOpenRemote, m_btnDelete })
        b->setEnabled(true);
    m_btnConnect->setEnabled(false);
    rebuildTree();
}

void SftpPanel::onDisconnect()
{
    m_tree->clear();
    m_pathLabel->setText(tr("(desconectado)"));
    for (auto* b : { m_btnDisconnect, m_btnRefresh, m_btnCdUp, m_btnUpload,
                     m_btnDownload, m_btnOpenRemote, m_btnDelete })
        b->setEnabled(false);
    m_btnConnect->setEnabled(true);
    m_currentHost.clear();
}

void SftpPanel::onRefresh() { rebuildTree(); }

void SftpPanel::rebuildTree()
{
    if (!m_client) return;
    QList<FtpEntry> entries;
    QString err;
    if (!m_client->listDir(QString(), entries, &err)) {
        QMessageBox::warning(this, tr("SFTP"), err);
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
}

QString SftpPanel::currentRemoteSelection() const {
    auto items = m_tree->selectedItems();
    if (items.isEmpty()) return {};
    return items.first()->data(0, Qt::UserRole).toString();
}

void SftpPanel::onItemDoubleClicked(QTreeWidgetItem* item, int)
{
    if (!item || !m_client) return;
    const QString name = item->data(0, Qt::UserRole).toString();
    const bool isDir   = item->data(0, Qt::UserRole + 1).toBool();
    if (isDir) {
        m_client->setCurrentDir(m_client->resolveAgainstCwd(name));
        rebuildTree();
    } else {
        onOpenRemote();
    }
}

void SftpPanel::onUpload()
{
    if (!m_client) return;
    const QString src = QFileDialog::getOpenFileName(this, tr("Upload via SFTP"));
    if (src.isEmpty()) return;
    const QString remote = m_client->resolveAgainstCwd(QFileInfo(src).fileName());
    QString err;
    if (!m_client->store(src, remote, &err)) {
        QMessageBox::warning(this, tr("SFTP"), err);
    }
    rebuildTree();
}

void SftpPanel::onDownload()
{
    if (!m_client) return;
    const QString name = currentRemoteSelection();
    if (name.isEmpty()) return;
    const QString dest = QFileDialog::getSaveFileName(this, tr("Download de %1").arg(name), name);
    if (dest.isEmpty()) return;
    QString err;
    const QString abs = m_client->resolveAgainstCwd(name);
    if (!m_client->retrieve(abs, dest, &err)) {
        QMessageBox::warning(this, tr("SFTP"), err);
    }
}

void SftpPanel::onOpenRemote()
{
    if (!m_client) return;
    const QString name = currentRemoteSelection();
    if (name.isEmpty()) return;
    const QString abs = m_client->resolveAgainstCwd(name);
    const QString local = cacheLocalFor(abs);

    QString err;
    if (!m_client->retrieve(abs, local, &err)) {
        QMessageBox::warning(this, tr("SFTP"), err);
        return;
    }
    m_localToRemote.insert(local, abs);
    emit openLocalFile(local);
}

void SftpPanel::onDelete()
{
    if (!m_client) return;
    const QString name = currentRemoteSelection();
    if (name.isEmpty()) return;
    if (QMessageBox::question(this, tr("SFTP"),
            tr("Apagar '%1' no servidor?").arg(name)) != QMessageBox::Yes) return;
    QString err;
    if (!m_client->deleteFile(m_client->resolveAgainstCwd(name), &err)) {
        QMessageBox::warning(this, tr("SFTP"), err);
    }
    rebuildTree();
}

void SftpPanel::onCdUp()
{
    if (!m_client) return;
    QString cur = m_client->currentDir();
    if (cur.isEmpty() || cur == "/") return;
    if (cur.endsWith('/')) cur.chop(1);
    int slash = cur.lastIndexOf('/');
    cur = (slash <= 0) ? QStringLiteral("/") : cur.left(slash);
    m_client->setCurrentDir(cur);
    rebuildTree();
}

bool SftpPanel::isRemoteCachedFile(const QString& localPath, QString* outRemotePath) const
{
    auto it = m_localToRemote.constFind(localPath);
    if (it == m_localToRemote.constEnd()) return false;
    if (outRemotePath) *outRemotePath = it.value();
    return true;
}

void SftpPanel::commitRemoteSave(const QString& localPath)
{
    if (!m_client) return;
    auto it = m_localToRemote.constFind(localPath);
    if (it == m_localToRemote.constEnd()) return;
    QString err;
    if (!m_client->store(localPath, it.value(), &err)) {
        QMessageBox::warning(this, tr("SFTP — re-upload"), err);
        return;
    }
    emit statusMessage(tr("SFTP: '%1' atualizado.").arg(it.value()));
}
