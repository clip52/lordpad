#include "PortScanPanel.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

PortScanPanel::PortScanPanel(QWidget* parent) : QDockWidget(tr("Portas"), parent)
{
    setObjectName(QStringLiteral("PortScanPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    m_host = new QLineEdit(root); m_host->setText(QStringLiteral("127.0.0.1"));
    m_ports = new QLineEdit(root); m_ports->setText(QStringLiteral("1-1024,3000,8080,8000-8100"));
    m_localBtn = new QPushButton(tr("Listeners locais (ss)"), root);
    m_scanBtn  = new QPushButton(tr("Scan TCP"), root);
    m_tree = new QTreeWidget(root);
    m_tree->setHeaderLabels({ tr("Endereço"), tr("Porta"), tr("Estado/Proc") });
    m_tree->setRootIsDecorated(false);
    m_status = new QLabel(root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("Host:"), root));
    row1->addWidget(m_host);
    row1->addWidget(new QLabel(tr("Portas:"), root));
    row1->addWidget(m_ports, 1);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(m_localBtn);
    row2->addWidget(m_scanBtn);
    row2->addStretch(1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(m_tree, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_localBtn, &QPushButton::clicked, this, &PortScanPanel::onLocalListeners);
    connect(m_scanBtn,  &QPushButton::clicked, this, &PortScanPanel::onScanRange);
}

void PortScanPanel::onLocalListeners()
{
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("ss"));
    if (tool.isEmpty()) { m_status->setText(tr("ss não encontrado.")); return; }
    QProcess p;
    p.start(tool, { QStringLiteral("-tlnp") });
    if (!p.waitForFinished(8000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }
    m_tree->clear();
    const QStringList lines = QString::fromUtf8(p.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
    for (int i = 1; i < lines.size(); ++i) {  // skip header
        const QStringList c = lines[i].split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (c.size() < 5) continue;
        const QString local = c[3];
        const int colon = local.lastIndexOf(':');
        if (colon < 0) continue;
        auto* it = new QTreeWidgetItem(m_tree);
        it->setText(0, local.left(colon));
        it->setText(1, local.mid(colon + 1));
        it->setText(2, c.size() >= 6 ? c[5] : c.last());
    }
    for (int c = 0; c < 3; ++c) m_tree->resizeColumnToContents(c);
    m_status->setText(tr("%1 listeners").arg(m_tree->topLevelItemCount()));
}

namespace {
QList<int> expandPortSpec(const QString& spec)
{
    QList<int> out;
    for (const QString& part : spec.split(',', Qt::SkipEmptyParts)) {
        const QString p = part.trimmed();
        if (p.contains('-')) {
            const int a = p.section('-', 0, 0).toInt();
            const int b = p.section('-', 1, 1).toInt();
            for (int i = a; i <= b && i > 0 && i < 65536; ++i) out << i;
        } else {
            const int i = p.toInt();
            if (i > 0 && i < 65536) out << i;
        }
    }
    return out;
}
} // namespace

void PortScanPanel::onScanRange()
{
    m_tree->clear();
    const QString host = m_host->text().trimmed();
    if (host.isEmpty()) return;
    const QList<int> ports = expandPortSpec(m_ports->text());
    if (ports.isEmpty()) { m_status->setText(tr("Spec de portas vazia.")); return; }
    if (ports.size() > 4096) { m_status->setText(tr("Limit 4096 portas.")); return; }

    int open = 0;
    m_status->setText(tr("Scanning %1 portas…").arg(ports.size()));
    qApp->processEvents();
    for (int port : ports) {
        QTcpSocket s;
        s.connectToHost(host, static_cast<quint16>(port));
        if (s.waitForConnected(150)) {
            auto* it = new QTreeWidgetItem(m_tree);
            it->setText(0, host);
            it->setText(1, QString::number(port));
            it->setText(2, QStringLiteral("open"));
            ++open;
            s.disconnectFromHost();
        }
    }
    for (int c = 0; c < 3; ++c) m_tree->resizeColumnToContents(c);
    m_status->setText(tr("%1 abertas / %2 testadas").arg(open).arg(ports.size()));
}
