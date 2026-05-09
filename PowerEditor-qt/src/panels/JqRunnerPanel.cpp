#include "JqRunnerPanel.h"

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWidget>

#include <ScintillaEdit.h>

JqRunnerPanel::JqRunnerPanel(QWidget* parent) : QDockWidget(tr("jq"), parent)
{
    setObjectName(QStringLiteral("JqRunnerPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_query = new QLineEdit(root); m_query->setText(QStringLiteral("."));
    m_query->setFont(mono);
    m_runBtn = new QPushButton(tr("Run"), root);
    m_writeBtn = new QPushButton(tr("Escrever no buffer"), root);
    m_out = new QPlainTextEdit(root); m_out->setReadOnly(true); m_out->setFont(mono);
    m_status = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(new QLabel(tr("Query:"), root));
    row->addWidget(m_query, 1);
    row->addWidget(m_runBtn);
    row->addWidget(m_writeBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row);
    lay->addWidget(m_out, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_runBtn, &QPushButton::clicked, this, &JqRunnerPanel::onRun);
    connect(m_query, &QLineEdit::returnPressed, this, &JqRunnerPanel::onRun);
    connect(m_writeBtn, &QPushButton::clicked, this, &JqRunnerPanel::onWriteBuffer);
}

void JqRunnerPanel::onRun()
{
    if (!m_editor) { m_status->setText(tr("Sem editor.")); return; }
    const QString jq = QStandardPaths::findExecutable(QStringLiteral("jq"));
    if (jq.isEmpty()) { m_status->setText(tr("jq não encontrado.")); return; }
    QByteArray bytes = m_editor->getText(m_editor->textLength() + 1);
    if (!bytes.isEmpty() && bytes.endsWith('\0')) bytes.chop(1);

    QProcess p;
    p.setProcessChannelMode(QProcess::SeparateChannels);
    p.start(jq, { m_query->text() });
    p.write(bytes);
    p.closeWriteChannel();
    if (!p.waitForFinished(10000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }
    if (p.exitCode() == 0) {
        m_out->setPlainText(QString::fromUtf8(p.readAllStandardOutput()));
        m_status->setText(tr("OK"));
    } else {
        m_out->setPlainText(QString::fromUtf8(p.readAllStandardError()));
        m_status->setText(tr("Erro jq"));
    }
}

void JqRunnerPanel::onWriteBuffer()
{
    if (!m_editor) { m_status->setText(tr("Sem editor.")); return; }
    const QByteArray bytes = m_out->toPlainText().toUtf8();
    m_editor->beginUndoAction();
    m_editor->setText(bytes.constData());
    m_editor->endUndoAction();
    m_status->setText(tr("Buffer atualizado."));
}
