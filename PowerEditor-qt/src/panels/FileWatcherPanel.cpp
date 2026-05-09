#include "FileWatcherPanel.h"

#include <QDateTime>
#include <QFile>
#include <QFileSystemWatcher>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QWidget>

FileWatcherPanel::FileWatcherPanel(QWidget* parent) : QDockWidget(tr("File watcher"), parent)
{
    setObjectName(QStringLiteral("FileWatcherPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_pathEdit = new QLineEdit(root); m_pathEdit->setPlaceholderText(QStringLiteral("/path/a/observar"));
    m_cmdEdit  = new QLineEdit(root); m_cmdEdit->setPlaceholderText(tr("comando shell a rodar"));
    m_addBtn   = new QPushButton(tr("+"), root);
    m_delBtn   = new QPushButton(tr("−"), root);
    m_rulesList = new QListWidget(root);
    m_log = new QPlainTextEdit(root); m_log->setReadOnly(true); m_log->setFont(mono);
    m_status = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(m_pathEdit, 2);
    row->addWidget(m_cmdEdit,  3);
    row->addWidget(m_addBtn);
    row->addWidget(m_delBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row);
    lay->addWidget(m_rulesList, 1);
    lay->addWidget(new QLabel(tr("Log:"), root));
    lay->addWidget(m_log, 2);
    lay->addWidget(m_status);
    setWidget(root);

    m_watcher = new QFileSystemWatcher(this);
    connect(m_watcher, &QFileSystemWatcher::fileChanged,
            this, &FileWatcherPanel::onTriggered);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &FileWatcherPanel::onTriggered);
    connect(m_addBtn, &QPushButton::clicked, this, &FileWatcherPanel::onAddRule);
    connect(m_delBtn, &QPushButton::clicked, this, &FileWatcherPanel::onRemoveRule);

    load();
}

void FileWatcherPanel::onAddRule()
{
    const QString p = m_pathEdit->text().trimmed();
    const QString c = m_cmdEdit->text().trimmed();
    if (p.isEmpty() || c.isEmpty()) return;
    m_rules.insert(p, c);
    m_watcher->addPath(p);
    auto* it = new QListWidgetItem(QStringLiteral("%1  →  %2").arg(p, c), m_rulesList);
    it->setData(Qt::UserRole, p);
    m_pathEdit->clear(); m_cmdEdit->clear();
    persist();
}

void FileWatcherPanel::onRemoveRule()
{
    auto* it = m_rulesList->currentItem();
    if (!it) return;
    const QString p = it->data(Qt::UserRole).toString();
    m_rules.remove(p);
    m_watcher->removePath(p);
    delete m_rulesList->takeItem(m_rulesList->row(it));
    persist();
}

void FileWatcherPanel::onTriggered(const QString& path)
{
    const QString cmd = m_rules.value(path);
    if (cmd.isEmpty()) return;
    m_log->appendPlainText(tr("[%1] %2 → %3").arg(
        QDateTime::currentDateTime().toString(Qt::ISODate), path, cmd));
    QProcess::startDetached(QStringLiteral("/bin/sh"), { QStringLiteral("-c"), cmd });
    // Some editors save by replace+rename — re-add path after a brief wait
    // so we keep watching post-rename.
    if (QFile::exists(path) && !m_watcher->files().contains(path) && !m_watcher->directories().contains(path))
        m_watcher->addPath(path);
}

void FileWatcherPanel::persist() const
{
    QSettings s; s.beginGroup(QStringLiteral("FileWatcher"));
    s.remove(QString());
    for (auto it = m_rules.constBegin(); it != m_rules.constEnd(); ++it)
        s.setValue(it.key(), it.value());
    s.endGroup();
}
void FileWatcherPanel::load()
{
    QSettings s; s.beginGroup(QStringLiteral("FileWatcher"));
    for (const QString& k : s.allKeys()) {
        const QString v = s.value(k).toString();
        m_rules.insert(k, v);
        m_watcher->addPath(k);
        auto* it = new QListWidgetItem(QStringLiteral("%1  →  %2").arg(k, v), m_rulesList);
        it->setData(Qt::UserRole, k);
    }
    s.endGroup();
}
