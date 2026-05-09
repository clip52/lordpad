#include "DockerPanel.h"

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSplitter>
#include <QStandardPaths>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

DockerPanel::DockerPanel(QWidget* parent) : QDockWidget(tr("Docker"), parent)
{
    setObjectName(QStringLiteral("DockerPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_refreshBtn = new QPushButton(tr("Atualizar"), root);
    m_logsBtn    = new QPushButton(tr("Logs"), root);
    m_execBtn    = new QPushButton(tr("Exec"), root);
    m_startBtn   = new QPushButton(tr("Start"), root);
    m_stopBtn    = new QPushButton(tr("Stop"), root);
    m_execCmd = new QLineEdit(root);
    m_execCmd->setPlaceholderText(QStringLiteral("ex: sh -c 'env | sort'"));

    m_list = new QTreeWidget(root);
    m_list->setHeaderLabels({ QStringLiteral("ID"), QStringLiteral("Image"),
                               QStringLiteral("Status"), QStringLiteral("Names") });
    m_list->setRootIsDecorated(false);

    m_out = new QPlainTextEdit(root); m_out->setReadOnly(true); m_out->setFont(mono);
    m_status = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(m_refreshBtn);
    row->addWidget(m_startBtn);
    row->addWidget(m_stopBtn);
    row->addWidget(m_logsBtn);
    row->addStretch(1);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(m_execCmd, 1);
    row2->addWidget(m_execBtn);

    auto* split = new QSplitter(Qt::Vertical, root);
    split->addWidget(m_list); split->addWidget(m_out);
    split->setStretchFactor(0, 1); split->setStretchFactor(1, 2);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row);
    lay->addWidget(split, 1);
    lay->addLayout(row2);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_refreshBtn, &QPushButton::clicked, this, &DockerPanel::onRefresh);
    connect(m_logsBtn,    &QPushButton::clicked, this, &DockerPanel::onLogs);
    connect(m_execBtn,    &QPushButton::clicked, this, &DockerPanel::onExec);
    connect(m_startBtn,   &QPushButton::clicked, this, &DockerPanel::onStart);
    connect(m_stopBtn,    &QPushButton::clicked, this, &DockerPanel::onStop);

    onRefresh();
}

QString DockerPanel::currentContainerId() const
{
    auto* it = m_list->currentItem();
    return it ? it->text(0) : QString();
}

void DockerPanel::onRefresh()
{
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("docker"));
    if (tool.isEmpty()) { m_status->setText(tr("docker não encontrado.")); return; }
    QProcess p;
    p.start(tool, { QStringLiteral("ps"), QStringLiteral("-a"),
                     QStringLiteral("--format"),
                     QStringLiteral("{{.ID}}\t{{.Image}}\t{{.Status}}\t{{.Names}}") });
    if (!p.waitForFinished(8000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }
    m_list->clear();
    const QStringList lines = QString::fromUtf8(p.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
    for (const QString& l : lines) {
        const QStringList c = l.split('\t');
        if (c.size() < 4) continue;
        auto* it = new QTreeWidgetItem(m_list);
        for (int i = 0; i < 4; ++i) it->setText(i, c[i]);
    }
    for (int c = 0; c < 4; ++c) m_list->resizeColumnToContents(c);
    m_status->setText(tr("%1 containers").arg(lines.size()));
}

void DockerPanel::onLogs()
{
    const QString id = currentContainerId();
    if (id.isEmpty()) return;
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("docker"));
    if (tool.isEmpty()) return;

    if (m_logsProc) { m_logsProc->kill(); m_logsProc->deleteLater(); }
    m_logsProc = new QProcess(this);
    m_logsProc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_logsProc.data(), &QProcess::readyReadStandardOutput, this, [this]() {
        m_out->appendPlainText(QString::fromUtf8(m_logsProc->readAllStandardOutput()).trimmed());
    });
    m_out->clear();
    m_logsProc->start(tool, { QStringLiteral("logs"), QStringLiteral("--tail=200"),
                                QStringLiteral("-f"), id });
    m_status->setText(tr("Tailing logs %1").arg(id));
}

void DockerPanel::onExec()
{
    const QString id = currentContainerId();
    if (id.isEmpty()) return;
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("docker"));
    if (tool.isEmpty()) return;
    QStringList args = { QStringLiteral("exec"), id };
    args += QProcess::splitCommand(m_execCmd->text());
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(tool, args);
    if (!p.waitForFinished(15000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }
    m_out->appendPlainText("$ " + m_execCmd->text());
    m_out->appendPlainText(QString::fromUtf8(p.readAllStandardOutput()));
}

void DockerPanel::onStart()
{
    const QString id = currentContainerId();
    if (id.isEmpty()) return;
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("docker"));
    if (tool.isEmpty()) return;
    QProcess p;
    p.start(tool, { QStringLiteral("start"), id });
    p.waitForFinished(8000);
    onRefresh();
}

void DockerPanel::onStop()
{
    const QString id = currentContainerId();
    if (id.isEmpty()) return;
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("docker"));
    if (tool.isEmpty()) return;
    QProcess p;
    p.start(tool, { QStringLiteral("stop"), id });
    p.waitForFinished(15000);
    onRefresh();
}
