#include "LogTailPanel.h"

#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWidget>

LogTailPanel::LogTailPanel(QWidget* parent) : QDockWidget(tr("Log Tail"), parent)
{
    setObjectName(QStringLiteral("LogTailPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_pathEdit = new QLineEdit(root); m_pathEdit->setPlaceholderText(QStringLiteral("/var/log/syslog"));
    m_pickBtn = new QPushButton(tr("…"), root);
    m_addBtn  = new QPushButton(tr("+ tail"), root);
    m_delBtn  = new QPushButton(tr("− remover"), root);
    m_clearBtn = new QPushButton(tr("Limpar"), root);
    m_files = new QListWidget(root); m_files->setMaximumWidth(280);
    m_out = new QPlainTextEdit(root); m_out->setReadOnly(true); m_out->setFont(mono);
    m_status = new QLabel(root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(m_pathEdit, 1);
    row1->addWidget(m_pickBtn);
    row1->addWidget(m_addBtn);
    row1->addWidget(m_delBtn);
    row1->addWidget(m_clearBtn);

    auto* split = new QSplitter(Qt::Horizontal, root);
    split->addWidget(m_files); split->addWidget(m_out);
    split->setStretchFactor(0, 0); split->setStretchFactor(1, 1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row1);
    lay->addWidget(split, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_pickBtn,  &QPushButton::clicked, this, &LogTailPanel::onPickFile);
    connect(m_addBtn,   &QPushButton::clicked, this, &LogTailPanel::onAdd);
    connect(m_delBtn,   &QPushButton::clicked, this, &LogTailPanel::onRemove);
    connect(m_clearBtn, &QPushButton::clicked, this, &LogTailPanel::onClear);

    load();
}

LogTailPanel::~LogTailPanel()
{
    for (auto* p : m_procs) { if (p) { p->kill(); p->waitForFinished(500); p->deleteLater(); } }
}

void LogTailPanel::onPickFile()
{
    const QString p = QFileDialog::getOpenFileName(this, tr("Log file"), QString(), tr("Todos (*)"));
    if (!p.isEmpty()) m_pathEdit->setText(p);
}

void LogTailPanel::onAdd()
{
    const QString path = m_pathEdit->text().trimmed();
    if (path.isEmpty()) return;
    if (m_procs.contains(path)) { m_status->setText(tr("Já tailing.")); return; }
    if (!QFileInfo::exists(path)) { m_status->setText(tr("Arquivo não existe.")); return; }
    m_files->addItem(path);
    startTailFor(path);
    m_pathEdit->clear();
    persist();
}

void LogTailPanel::onRemove()
{
    auto* it = m_files->currentItem();
    if (!it) return;
    const QString path = it->text();
    if (auto* p = m_procs.value(path, nullptr)) {
        p->kill(); p->deleteLater(); m_procs.remove(path);
    }
    delete m_files->takeItem(m_files->row(it));
    persist();
}

void LogTailPanel::onClear() { m_out->clear(); }

void LogTailPanel::startTailFor(const QString& path)
{
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("tail"));
    if (tool.isEmpty()) { m_status->setText(tr("tail não encontrado.")); return; }
    auto* p = new QProcess(this);
    p->setProcessChannelMode(QProcess::MergedChannels);
    const QString tag = QFileInfo(path).fileName();
    connect(p, &QProcess::readyReadStandardOutput, this, [this, p, tag]() {
        const QStringList lines = QString::fromUtf8(p->readAllStandardOutput()).split('\n');
        for (const QString& l : lines) {
            if (l.isEmpty()) continue;
            const QString ts = QDateTime::currentDateTime().toString(QStringLiteral("hh:mm:ss"));
            m_out->appendPlainText(QStringLiteral("[%1] [%2] %3").arg(ts, tag, l));
        }
    });
    p->start(tool, { QStringLiteral("-F"), QStringLiteral("-n"), QStringLiteral("50"), path });
    m_procs.insert(path, p);
    m_status->setText(tr("Tailing %1 arquivo(s)").arg(m_procs.size()));
}

void LogTailPanel::persist() const
{
    QStringList paths;
    for (int i = 0; i < m_files->count(); ++i) paths << m_files->item(i)->text();
    QSettings().setValue(QStringLiteral("LogTail/files"), paths);
}

void LogTailPanel::load()
{
    QSettings s;
    for (const QString& p : s.value(QStringLiteral("LogTail/files")).toStringList()) {
        if (!QFileInfo::exists(p)) continue;
        m_files->addItem(p);
        startTailFor(p);
    }
}
