#include "GpgPanel.h"

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

#include <ScintillaEdit.h>

namespace {
QString gpgPath() { return QStandardPaths::findExecutable(QStringLiteral("gpg")); }
} // namespace

GpgPanel::GpgPanel(QWidget* parent) : QDockWidget(tr("GPG"), parent)
{
    setObjectName(QStringLiteral("GpgPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_recipient = new QLineEdit(root);
    m_recipient->setPlaceholderText(QStringLiteral("destinatário (email/keyid)"));
    m_listBtn = new QPushButton(tr("Listar chaves"), root);
    m_encBtn  = new QPushButton(tr("Encrypt buffer"), root);
    m_decBtn  = new QPushButton(tr("Decrypt buffer"), root);
    m_signBtn = new QPushButton(tr("Sign"), root);
    m_verifyBtn = new QPushButton(tr("Verify"), root);

    m_keys = new QTreeWidget(root);
    m_keys->setHeaderLabels({ tr("Tipo"), tr("Tamanho"), tr("ID"), tr("UID") });
    m_keys->setRootIsDecorated(false);
    m_keys->setMaximumHeight(150);

    m_out = new QPlainTextEdit(root); m_out->setReadOnly(true); m_out->setFont(mono);
    m_status = new QLabel(root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("Para:"), root));
    row1->addWidget(m_recipient, 1);
    row1->addWidget(m_listBtn);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(m_encBtn);
    row2->addWidget(m_decBtn);
    row2->addWidget(m_signBtn);
    row2->addWidget(m_verifyBtn);
    row2->addStretch(1);

    auto* split = new QSplitter(Qt::Vertical, root);
    split->addWidget(m_keys);
    split->addWidget(m_out);
    split->setStretchFactor(0, 1); split->setStretchFactor(1, 2);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(split, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_listBtn,   &QPushButton::clicked, this, &GpgPanel::onListKeys);
    connect(m_encBtn,    &QPushButton::clicked, this, &GpgPanel::onEncrypt);
    connect(m_decBtn,    &QPushButton::clicked, this, &GpgPanel::onDecrypt);
    connect(m_signBtn,   &QPushButton::clicked, this, &GpgPanel::onSign);
    connect(m_verifyBtn, &QPushButton::clicked, this, &GpgPanel::onVerify);

    if (gpgPath().isEmpty()) m_status->setText(tr("gpg não encontrado no PATH."));
}

void GpgPanel::onListKeys()
{
    const QString tool = gpgPath();
    if (tool.isEmpty()) { m_status->setText(tr("gpg ausente.")); return; }
    QProcess p;
    p.start(tool, { QStringLiteral("--list-keys"), QStringLiteral("--with-colons") });
    if (!p.waitForFinished(8000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }
    m_keys->clear();
    QTreeWidgetItem* parent = nullptr;
    for (const QString& l : QString::fromUtf8(p.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts)) {
        const QStringList c = l.split(':');
        if (c.isEmpty()) continue;
        if (c[0] == QStringLiteral("pub") || c[0] == QStringLiteral("sec")) {
            parent = new QTreeWidgetItem(m_keys);
            parent->setText(0, c[0]);
            parent->setText(1, c.value(2));
            parent->setText(2, c.value(4));
        } else if (c[0] == QStringLiteral("uid") && parent) {
            const QString prev = parent->text(3);
            parent->setText(3, prev.isEmpty() ? c.value(9) : prev);
        }
    }
    for (int c = 0; c < 4; ++c) m_keys->resizeColumnToContents(c);
    m_status->setText(tr("%1 chaves").arg(m_keys->topLevelItemCount()));
}

void GpgPanel::onEncrypt()
{
    if (!m_editor) return;
    const QString tool = gpgPath();
    if (tool.isEmpty()) { m_status->setText(tr("gpg ausente.")); return; }
    if (m_recipient->text().trimmed().isEmpty()) {
        m_status->setText(tr("Recipiente obrigatório.")); return;
    }
    QByteArray bytes = m_editor->getText(m_editor->textLength() + 1);
    if (!bytes.isEmpty() && bytes.endsWith('\0')) bytes.chop(1);

    QProcess p;
    p.setProcessChannelMode(QProcess::SeparateChannels);
    p.start(tool, { QStringLiteral("--armor"), QStringLiteral("--encrypt"),
                     QStringLiteral("--trust-model"), QStringLiteral("always"),
                     QStringLiteral("-r"), m_recipient->text().trimmed() });
    p.write(bytes); p.closeWriteChannel();
    if (!p.waitForFinished(20000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }
    if (p.exitCode() != 0) {
        m_out->setPlainText(QString::fromUtf8(p.readAllStandardError()));
        m_status->setText(tr("Falhou."));
        return;
    }
    m_editor->beginUndoAction();
    m_editor->setText(p.readAllStandardOutput().constData());
    m_editor->endUndoAction();
    m_status->setText(tr("Buffer cifrado."));
}

void GpgPanel::onDecrypt()
{
    if (!m_editor) return;
    const QString tool = gpgPath();
    if (tool.isEmpty()) { m_status->setText(tr("gpg ausente.")); return; }
    QByteArray bytes = m_editor->getText(m_editor->textLength() + 1);
    if (!bytes.isEmpty() && bytes.endsWith('\0')) bytes.chop(1);

    QProcess p;
    p.setProcessChannelMode(QProcess::SeparateChannels);
    p.start(tool, { QStringLiteral("--decrypt") });
    p.write(bytes); p.closeWriteChannel();
    if (!p.waitForFinished(20000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }
    if (p.exitCode() != 0) {
        m_out->setPlainText(QString::fromUtf8(p.readAllStandardError()));
        m_status->setText(tr("Falhou."));
        return;
    }
    m_editor->beginUndoAction();
    m_editor->setText(p.readAllStandardOutput().constData());
    m_editor->endUndoAction();
    m_status->setText(tr("Buffer decifrado."));
}

void GpgPanel::onSign()
{
    if (!m_editor) return;
    const QString tool = gpgPath();
    if (tool.isEmpty()) return;
    QByteArray bytes = m_editor->getText(m_editor->textLength() + 1);
    if (!bytes.isEmpty() && bytes.endsWith('\0')) bytes.chop(1);
    QProcess p;
    p.setProcessChannelMode(QProcess::SeparateChannels);
    p.start(tool, { QStringLiteral("--armor"), QStringLiteral("--clearsign") });
    p.write(bytes); p.closeWriteChannel();
    if (!p.waitForFinished(15000)) { p.kill(); return; }
    if (p.exitCode() != 0) {
        m_out->setPlainText(QString::fromUtf8(p.readAllStandardError()));
        m_status->setText(tr("Falhou.")); return;
    }
    m_editor->beginUndoAction();
    m_editor->setText(p.readAllStandardOutput().constData());
    m_editor->endUndoAction();
    m_status->setText(tr("Assinado."));
}

void GpgPanel::onVerify()
{
    if (!m_editor) return;
    const QString tool = gpgPath();
    if (tool.isEmpty()) return;
    QByteArray bytes = m_editor->getText(m_editor->textLength() + 1);
    if (!bytes.isEmpty() && bytes.endsWith('\0')) bytes.chop(1);
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(tool, { QStringLiteral("--verify") });
    p.write(bytes); p.closeWriteChannel();
    if (!p.waitForFinished(15000)) { p.kill(); return; }
    m_out->setPlainText(QString::fromUtf8(p.readAllStandardOutput()));
    m_status->setText(p.exitCode() == 0 ? tr("Verificado OK.") : tr("Falha na verificação."));
}
