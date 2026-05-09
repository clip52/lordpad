#include "ProfileRunnerPanel.h"

#include <QComboBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QWidget>

ProfileRunnerPanel::ProfileRunnerPanel(QWidget* parent) : QDockWidget(tr("Profiler"), parent)
{
    setObjectName(QStringLiteral("ProfileRunnerPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_provider = new QComboBox(root);
    m_provider->addItems({ QStringLiteral("py-spy"), QStringLiteral("perf") });
    m_command = new QLineEdit(root);
    m_command->setPlaceholderText(QStringLiteral("python my_script.py    OR   ./my_program"));
    m_duration = new QLineEdit(root);
    m_duration->setPlaceholderText(tr("segundos (default 10)"));
    m_duration->setText(QStringLiteral("10"));
    m_runBtn = new QPushButton(tr("Profile"), root);
    m_output = new QPlainTextEdit(root);
    m_output->setReadOnly(true);
    m_output->setFont(mono);
    m_status = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(new QLabel(tr("Tool:"), root));
    row->addWidget(m_provider);
    row->addWidget(new QLabel(tr("Comando:"), root));
    row->addWidget(m_command, 1);
    row->addWidget(new QLabel(tr("Dur.:"), root));
    row->addWidget(m_duration);
    row->addWidget(m_runBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row);
    lay->addWidget(m_output, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_runBtn, &QPushButton::clicked, this, &ProfileRunnerPanel::onRun);
}

void ProfileRunnerPanel::onRun()
{
    const QString tool = m_provider->currentText();
    const QString exe  = QStandardPaths::findExecutable(tool);
    if (exe.isEmpty()) { m_status->setText(tr("`%1` não encontrado no PATH.").arg(tool)); return; }
    const QStringList cmd = QProcess::splitCommand(m_command->text().trimmed());
    if (cmd.isEmpty()) { m_status->setText(tr("Comando vazio.")); return; }
    const int dur = m_duration->text().toInt();
    QTemporaryDir tmp; tmp.setAutoRemove(true);
    if (!tmp.isValid()) { m_status->setText(tr("temp dir.")); return; }

    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    QStringList args;
    if (tool == QStringLiteral("py-spy")) {
        // py-spy record com flamegraph em texto.
        const QString out = tmp.path() + "/spy.txt";
        args << QStringLiteral("record")
             << QStringLiteral("-d") << QString::number(qMax(1, dur))
             << QStringLiteral("-f") << QStringLiteral("raw")
             << QStringLiteral("-o") << out
             << QStringLiteral("--") << cmd;
        m_status->setText(tr("Coletando %1s…").arg(dur));
        p.start(exe, args);
        p.waitForFinished((dur + 30) * 1000);
        QFile f(out);
        if (f.open(QIODevice::ReadOnly)) {
            m_output->setPlainText(QString::fromUtf8(f.readAll()));
            f.close();
        }
    } else {
        // perf record + perf report --stdio.
        const QString data = tmp.path() + "/perf.data";
        QProcess rec;
        QStringList recArgs = { QStringLiteral("record"), QStringLiteral("-F"), QStringLiteral("99"),
                                QStringLiteral("-g"), QStringLiteral("-o"), data,
                                QStringLiteral("--") };
        recArgs += cmd;
        rec.start(QStringLiteral("perf"), recArgs);
        rec.waitForFinished((dur + 30) * 1000);

        QProcess rep;
        rep.setProcessChannelMode(QProcess::MergedChannels);
        rep.start(QStringLiteral("perf"), { QStringLiteral("report"),
                                             QStringLiteral("--stdio"),
                                             QStringLiteral("-i"), data });
        rep.waitForFinished(30000);
        m_output->setPlainText(QString::fromUtf8(rep.readAllStandardOutput()));
    }
    m_status->setText(tr("Concluído."));
}
