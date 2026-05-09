#include "SystemdServicesPanel.h"

#include <QCheckBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QStandardPaths>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

SystemdServicesPanel::SystemdServicesPanel(QWidget* parent) : QDockWidget(tr("systemd"), parent)
{
    setObjectName(QStringLiteral("SystemdServicesPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_filter = new QLineEdit(root); m_filter->setPlaceholderText(tr("filtro de unidade"));
    m_userBox = new QCheckBox(tr("--user"), root);
    m_refreshBtn = new QPushButton(tr("Atualizar"), root);
    m_startBtn   = new QPushButton(tr("Start"), root);
    m_stopBtn    = new QPushButton(tr("Stop"), root);
    m_restartBtn = new QPushButton(tr("Restart"), root);
    m_statusBtn  = new QPushButton(tr("Status"), root);
    m_journalBtn = new QPushButton(tr("Journal -f"), root);

    m_units = new QTreeWidget(root);
    m_units->setHeaderLabels({ tr("Unit"), tr("Load"), tr("Active"), tr("Sub"), tr("Description") });
    m_units->setRootIsDecorated(false);

    m_out = new QPlainTextEdit(root); m_out->setReadOnly(true); m_out->setFont(mono);
    m_status = new QLabel(root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(m_filter, 1);
    row1->addWidget(m_userBox);
    row1->addWidget(m_refreshBtn);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(m_startBtn);
    row2->addWidget(m_stopBtn);
    row2->addWidget(m_restartBtn);
    row2->addWidget(m_statusBtn);
    row2->addWidget(m_journalBtn);
    row2->addStretch(1);

    auto* split = new QSplitter(Qt::Vertical, root);
    split->addWidget(m_units); split->addWidget(m_out);
    split->setStretchFactor(0, 1); split->setStretchFactor(1, 1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(split, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_refreshBtn, &QPushButton::clicked, this, &SystemdServicesPanel::onRefresh);
    connect(m_startBtn,   &QPushButton::clicked, this, &SystemdServicesPanel::onStart);
    connect(m_stopBtn,    &QPushButton::clicked, this, &SystemdServicesPanel::onStop);
    connect(m_restartBtn, &QPushButton::clicked, this, &SystemdServicesPanel::onRestart);
    connect(m_statusBtn,  &QPushButton::clicked, this, &SystemdServicesPanel::onStatus);
    connect(m_journalBtn, &QPushButton::clicked, this, &SystemdServicesPanel::onJournalTail);
    connect(m_filter, &QLineEdit::textChanged, this, [this]() {
        const QString f = m_filter->text();
        for (int i = 0; i < m_units->topLevelItemCount(); ++i) {
            auto* it = m_units->topLevelItem(i);
            it->setHidden(!f.isEmpty() && !it->text(0).contains(f, Qt::CaseInsensitive)
                                       && !it->text(4).contains(f, Qt::CaseInsensitive));
        }
    });

    onRefresh();
}

QStringList SystemdServicesPanel::ctlArgs() const
{
    return m_userBox->isChecked() ? QStringList{ QStringLiteral("--user") } : QStringList{};
}

QString SystemdServicesPanel::currentUnit() const
{
    auto* it = m_units->currentItem();
    return it ? it->text(0) : QString();
}

void SystemdServicesPanel::onRefresh()
{
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("systemctl"));
    if (tool.isEmpty()) { m_status->setText(tr("systemctl não encontrado.")); return; }
    QProcess p;
    QStringList args = ctlArgs();
    args << QStringLiteral("list-units") << QStringLiteral("--type=service")
         << QStringLiteral("--all") << QStringLiteral("--no-pager")
         << QStringLiteral("--no-legend") << QStringLiteral("--plain");
    p.start(tool, args);
    if (!p.waitForFinished(15000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }
    m_units->clear();
    for (const QString& l : QString::fromUtf8(p.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts)) {
        const QStringList c = l.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (c.size() < 4) continue;
        auto* it = new QTreeWidgetItem(m_units);
        it->setText(0, c[0]);
        it->setText(1, c[1]);
        it->setText(2, c[2]);
        it->setText(3, c[3]);
        if (c.size() > 4) it->setText(4, QStringList(c.mid(4)).join(' '));
    }
    for (int c = 0; c < 5; ++c) m_units->resizeColumnToContents(c);
    m_status->setText(tr("%1 unidades").arg(m_units->topLevelItemCount()));
}

static void runCtl(const QString& tool, const QStringList& base, const QString& cmd, const QString& unit)
{
    if (tool.isEmpty() || unit.isEmpty()) return;
    QProcess p;
    QStringList args = base; args << cmd << unit;
    p.start(tool, args);
    p.waitForFinished(20000);
}

void SystemdServicesPanel::onStart()
{
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("systemctl"));
    runCtl(tool, ctlArgs(), QStringLiteral("start"), currentUnit());
    onRefresh();
}
void SystemdServicesPanel::onStop()
{
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("systemctl"));
    runCtl(tool, ctlArgs(), QStringLiteral("stop"), currentUnit());
    onRefresh();
}
void SystemdServicesPanel::onRestart()
{
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("systemctl"));
    runCtl(tool, ctlArgs(), QStringLiteral("restart"), currentUnit());
    onRefresh();
}

void SystemdServicesPanel::onStatus()
{
    const QString unit = currentUnit();
    if (unit.isEmpty()) return;
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("systemctl"));
    if (tool.isEmpty()) return;
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    QStringList args = ctlArgs();
    args << QStringLiteral("status") << QStringLiteral("--no-pager") << unit;
    p.start(tool, args);
    p.waitForFinished(10000);
    m_out->setPlainText(QString::fromUtf8(p.readAllStandardOutput()));
}

void SystemdServicesPanel::onJournalTail()
{
    const QString unit = currentUnit();
    if (unit.isEmpty()) return;
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("journalctl"));
    if (tool.isEmpty()) { m_status->setText(tr("journalctl não encontrado.")); return; }
    if (m_journalProc) { m_journalProc->kill(); m_journalProc->deleteLater(); }
    m_journalProc = new QProcess(this);
    m_journalProc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_journalProc.data(), &QProcess::readyReadStandardOutput, this, [this]() {
        m_out->appendPlainText(QString::fromUtf8(m_journalProc->readAllStandardOutput()).trimmed());
    });
    QStringList args;
    if (m_userBox->isChecked()) args << QStringLiteral("--user");
    args << QStringLiteral("-u") << unit << QStringLiteral("-f") << QStringLiteral("-n") << QStringLiteral("100");
    m_out->clear();
    m_journalProc->start(tool, args);
    m_status->setText(tr("Tail journal %1").arg(unit));
}
