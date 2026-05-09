#include "KubectlPanel.h"

#include <QComboBox>
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

KubectlPanel::KubectlPanel(QWidget* parent) : QDockWidget(tr("kubectl"), parent)
{
    setObjectName(QStringLiteral("KubectlPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_ns = new QComboBox(root); m_ns->addItem(QStringLiteral("default"));
    m_nsRefreshBtn = new QPushButton(tr("⟲ ns"), root);
    m_refreshBtn = new QPushButton(tr("Pods"), root);
    m_logsBtn = new QPushButton(tr("Logs"), root);
    m_execBtn = new QPushButton(tr("Exec"), root);
    m_descBtn = new QPushButton(tr("Describe"), root);
    m_delBtn  = new QPushButton(tr("Delete"), root);
    m_execCmd = new QLineEdit(root);
    m_execCmd->setPlaceholderText(QStringLiteral("ex: sh -c 'env'"));

    m_pods = new QTreeWidget(root);
    m_pods->setHeaderLabels({ tr("Nome"), tr("Ready"), tr("Status"), tr("Restarts"), tr("Age") });
    m_pods->setRootIsDecorated(false);
    m_pods->setUniformRowHeights(true);

    m_out = new QPlainTextEdit(root); m_out->setReadOnly(true); m_out->setFont(mono);
    m_status = new QLabel(root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("ns:"), root)); row1->addWidget(m_ns);
    row1->addWidget(m_nsRefreshBtn);
    row1->addWidget(m_refreshBtn);
    row1->addStretch(1);
    row1->addWidget(m_descBtn);
    row1->addWidget(m_logsBtn);
    row1->addWidget(m_delBtn);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(m_execCmd, 1);
    row2->addWidget(m_execBtn);

    auto* split = new QSplitter(Qt::Vertical, root);
    split->addWidget(m_pods); split->addWidget(m_out);
    split->setStretchFactor(0, 1); split->setStretchFactor(1, 2);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row1);
    lay->addWidget(split, 1);
    lay->addLayout(row2);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_nsRefreshBtn, &QPushButton::clicked, this, &KubectlPanel::onRefreshNamespaces);
    connect(m_refreshBtn,   &QPushButton::clicked, this, &KubectlPanel::onRefreshPods);
    connect(m_logsBtn,      &QPushButton::clicked, this, &KubectlPanel::onLogs);
    connect(m_execBtn,      &QPushButton::clicked, this, &KubectlPanel::onExec);
    connect(m_descBtn,      &QPushButton::clicked, this, &KubectlPanel::onDescribe);
    connect(m_delBtn,       &QPushButton::clicked, this, &KubectlPanel::onDelete);

    onRefreshNamespaces();
}

QString KubectlPanel::currentNamespace() const { return m_ns->currentText(); }
QString KubectlPanel::currentPod() const
{
    auto* it = m_pods->currentItem();
    return it ? it->text(0) : QString();
}

void KubectlPanel::onRefreshNamespaces()
{
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("kubectl"));
    if (tool.isEmpty()) { m_status->setText(tr("kubectl não encontrado.")); return; }
    QProcess p;
    p.start(tool, { QStringLiteral("get"), QStringLiteral("ns"),
                     QStringLiteral("-o"), QStringLiteral("name") });
    if (!p.waitForFinished(8000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }
    m_ns->clear();
    for (const QString& l : QString::fromUtf8(p.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts)) {
        m_ns->addItem(l.section('/', 1));
    }
    if (m_ns->findText(QStringLiteral("default")) >= 0)
        m_ns->setCurrentText(QStringLiteral("default"));
    m_status->setText(tr("%1 namespaces").arg(m_ns->count()));
}

void KubectlPanel::onRefreshPods()
{
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("kubectl"));
    if (tool.isEmpty()) return;
    QProcess p;
    p.start(tool, { QStringLiteral("get"), QStringLiteral("pods"),
                     QStringLiteral("-n"), currentNamespace(),
                     QStringLiteral("--no-headers") });
    if (!p.waitForFinished(15000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }
    m_pods->clear();
    for (const QString& l : QString::fromUtf8(p.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts)) {
        const QStringList c = l.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (c.size() < 5) continue;
        auto* it = new QTreeWidgetItem(m_pods);
        for (int i = 0; i < 5; ++i) it->setText(i, c.value(i));
    }
    for (int c = 0; c < 5; ++c) m_pods->resizeColumnToContents(c);
    m_status->setText(tr("%1 pods em %2").arg(m_pods->topLevelItemCount()).arg(currentNamespace()));
}

void KubectlPanel::onLogs()
{
    const QString pod = currentPod();
    if (pod.isEmpty()) return;
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("kubectl"));
    if (tool.isEmpty()) return;

    if (m_logsProc) { m_logsProc->kill(); m_logsProc->deleteLater(); }
    m_logsProc = new QProcess(this);
    m_logsProc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_logsProc.data(), &QProcess::readyReadStandardOutput, this, [this]() {
        m_out->appendPlainText(QString::fromUtf8(m_logsProc->readAllStandardOutput()).trimmed());
    });
    m_out->clear();
    m_logsProc->start(tool, { QStringLiteral("logs"), QStringLiteral("--tail=200"),
                                QStringLiteral("-f"), QStringLiteral("-n"), currentNamespace(), pod });
    m_status->setText(tr("Tail logs %1").arg(pod));
}

void KubectlPanel::onExec()
{
    const QString pod = currentPod();
    if (pod.isEmpty()) return;
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("kubectl"));
    if (tool.isEmpty()) return;
    QStringList args = { QStringLiteral("exec"), QStringLiteral("-n"), currentNamespace(), pod, QStringLiteral("--") };
    args += QProcess::splitCommand(m_execCmd->text());
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(tool, args);
    if (!p.waitForFinished(20000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }
    m_out->appendPlainText("$ " + m_execCmd->text());
    m_out->appendPlainText(QString::fromUtf8(p.readAllStandardOutput()));
}

void KubectlPanel::onDescribe()
{
    const QString pod = currentPod();
    if (pod.isEmpty()) return;
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("kubectl"));
    if (tool.isEmpty()) return;
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(tool, { QStringLiteral("describe"), QStringLiteral("pod"),
                     QStringLiteral("-n"), currentNamespace(), pod });
    if (!p.waitForFinished(20000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }
    m_out->setPlainText(QString::fromUtf8(p.readAllStandardOutput()));
}

void KubectlPanel::onDelete()
{
    const QString pod = currentPod();
    if (pod.isEmpty()) return;
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("kubectl"));
    if (tool.isEmpty()) return;
    QProcess p;
    p.start(tool, { QStringLiteral("delete"), QStringLiteral("pod"),
                     QStringLiteral("-n"), currentNamespace(), pod });
    p.waitForFinished(15000);
    onRefreshPods();
}
