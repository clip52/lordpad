#include "CronEditorPanel.h"

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWidget>

CronEditorPanel::CronEditorPanel(QWidget* parent) : QDockWidget(tr("Cron"), parent)
{
    setObjectName(QStringLiteral("CronEditorPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_text = new QPlainTextEdit(root); m_text->setFont(mono);
    m_text->setPlaceholderText(QStringLiteral("# minute hour day month weekday command\n0 9 * * 1-5 /usr/bin/echo bom-dia"));
    m_loadBtn = new QPushButton(tr("Carregar (crontab -l)"), root);
    m_saveBtn = new QPushButton(tr("Salvar (crontab -)"),    root);
    m_status  = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(m_loadBtn); row->addWidget(m_saveBtn); row->addStretch(1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row);
    lay->addWidget(m_text, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_loadBtn, &QPushButton::clicked, this, &CronEditorPanel::onLoad);
    connect(m_saveBtn, &QPushButton::clicked, this, &CronEditorPanel::onSave);
}

void CronEditorPanel::onLoad()
{
    const QString exe = QStandardPaths::findExecutable(QStringLiteral("crontab"));
    if (exe.isEmpty()) { m_status->setText(tr("crontab não encontrado.")); return; }
    QProcess p;
    p.start(exe, { QStringLiteral("-l") });
    p.waitForFinished(5000);
    if (p.exitCode() == 0) {
        m_text->setPlainText(QString::fromUtf8(p.readAllStandardOutput()));
        m_status->setText(tr("Carregado."));
    } else if (p.exitCode() == 1) {
        // "no crontab" — texto vazio é estado válido.
        m_text->setPlainText(QString());
        m_status->setText(tr("(sem crontab — comece a digitar)"));
    } else {
        m_status->setText(QString::fromUtf8(p.readAllStandardError()).trimmed());
    }
}

void CronEditorPanel::onSave()
{
    const QString exe = QStandardPaths::findExecutable(QStringLiteral("crontab"));
    if (exe.isEmpty()) { m_status->setText(tr("crontab não encontrado.")); return; }
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(exe, { QStringLiteral("-") });
    if (!p.waitForStarted(2000)) { m_status->setText(tr("Falha ao iniciar.")); return; }
    p.write(m_text->toPlainText().toUtf8());
    p.closeWriteChannel();
    p.waitForFinished(5000);
    if (p.exitCode() != 0) {
        m_status->setText(QString::fromUtf8(p.readAllStandardOutput()).trimmed());
    } else {
        m_status->setText(tr("Salvo."));
    }
}
