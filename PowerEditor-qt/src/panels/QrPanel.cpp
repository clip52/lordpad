#include "QrPanel.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QFileDialog>
#include <QFile>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QVBoxLayout>
#include <QWidget>

QrPanel::QrPanel(QWidget* parent) : QDockWidget(tr("QR Code"), parent)
{
    setObjectName(QStringLiteral("QrPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);

    m_input = new QPlainTextEdit(root);
    m_input->setPlaceholderText(tr("Texto / URL pra codificar…"));

    m_eccLevel = new QComboBox(root);
    m_eccLevel->addItem(tr("L (recupera ~7%)"),   QStringLiteral("L"));
    m_eccLevel->addItem(tr("M (recupera ~15%)"),  QStringLiteral("M"));
    m_eccLevel->addItem(tr("Q (recupera ~25%)"),  QStringLiteral("Q"));
    m_eccLevel->addItem(tr("H (recupera ~30%)"),  QStringLiteral("H"));
    m_eccLevel->setCurrentIndex(1);

    m_genBtn  = new QPushButton(tr("Gerar"), root);
    m_saveBtn = new QPushButton(tr("Salvar PNG…"), root);
    m_copyBtn = new QPushButton(tr("Copiar imagem"), root);
    m_saveBtn->setEnabled(false);
    m_copyBtn->setEnabled(false);

    m_image  = new QLabel(root);
    m_image->setAlignment(Qt::AlignCenter);
    m_image->setMinimumSize(220, 220);
    m_image->setStyleSheet(QStringLiteral("QLabel { background: white; border: 1px solid #888; }"));

    m_status = new QLabel(root);
    m_status->setWordWrap(true);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("ECC:"), root));
    row1->addWidget(m_eccLevel);
    row1->addStretch(1);
    row1->addWidget(m_genBtn);
    row1->addWidget(m_saveBtn);
    row1->addWidget(m_copyBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addWidget(m_input, 1);
    lay->addLayout(row1);
    lay->addWidget(m_image, 2);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_genBtn,  &QPushButton::clicked, this, &QrPanel::onGenerate);
    connect(m_saveBtn, &QPushButton::clicked, this, &QrPanel::onSaveAs);
    connect(m_copyBtn, &QPushButton::clicked, this, &QrPanel::onCopyImage);

    // Friendly hint at startup if qrencode is missing.
    if (QStandardPaths::findExecutable(QStringLiteral("qrencode")).isEmpty()) {
        m_status->setText(tr(
            "ℹ️  `qrencode` não foi encontrado no PATH. Instale com:\n"
            "    Fedora:  sudo dnf install qrencode\n"
            "    Debian:  sudo apt install qrencode\n"
            "    Arch:    sudo pacman -S qrencode"));
    }
}

bool QrPanel::runQrencode(const QString& text, const QString& outPath, QString* outErr) const
{
    const QString exe = QStandardPaths::findExecutable(QStringLiteral("qrencode"));
    if (exe.isEmpty()) {
        if (outErr) *outErr = tr("qrencode não está instalado.");
        return false;
    }
    QStringList args;
    args << QStringLiteral("-t") << QStringLiteral("PNG")
         << QStringLiteral("-s") << QStringLiteral("8")
         << QStringLiteral("-l") << m_eccLevel->currentData().toString()
         << QStringLiteral("-o") << outPath;

    QProcess p;
    p.start(exe, args);
    if (!p.waitForStarted(2000)) {
        if (outErr) *outErr = tr("Não consegui iniciar qrencode."); return false;
    }
    p.write(text.toUtf8());
    p.closeWriteChannel();
    if (!p.waitForFinished(8000)) { p.kill(); p.waitForFinished(500);
        if (outErr) *outErr = tr("qrencode demorou demais."); return false;
    }
    if (p.exitCode() != 0) {
        if (outErr) *outErr = QString::fromUtf8(p.readAllStandardError()).trimmed();
        return false;
    }
    return true;
}

void QrPanel::onGenerate()
{
    const QString text = m_input->toPlainText();
    if (text.isEmpty()) { m_status->setText(tr("Texto vazio.")); return; }

    // Use a per-session tmp file so subsequent generations overwrite cleanly.
    if (m_lastPng.isEmpty()) {
        QTemporaryFile tmp(QDir::tempPath() + "/qr_XXXXXX.png");
        tmp.setAutoRemove(false);
        if (!tmp.open()) { m_status->setText(tr("Falha ao criar arquivo temp.")); return; }
        m_lastPng = tmp.fileName();
        tmp.close();
    }

    QString err;
    if (!runQrencode(text, m_lastPng, &err)) {
        m_status->setText(tr("Erro: %1").arg(err));
        return;
    }
    QPixmap pix(m_lastPng);
    if (pix.isNull()) { m_status->setText(tr("PNG inválido.")); return; }
    m_image->setPixmap(pix.scaled(m_image->size(), Qt::KeepAspectRatio,
                                  Qt::SmoothTransformation));
    m_status->setText(tr("OK — %1×%2 px").arg(pix.width()).arg(pix.height()));
    m_saveBtn->setEnabled(true);
    m_copyBtn->setEnabled(true);
}

void QrPanel::onSaveAs()
{
    if (m_lastPng.isEmpty()) return;
    const QString dest = QFileDialog::getSaveFileName(this, tr("Salvar PNG"),
        QStringLiteral("qrcode.png"), tr("PNG (*.png)"));
    if (dest.isEmpty()) return;
    QFile::remove(dest);
    if (!QFile::copy(m_lastPng, dest)) {
        m_status->setText(tr("Falha ao salvar."));
        return;
    }
    m_status->setText(tr("Salvo em %1").arg(dest));
}

void QrPanel::onCopyImage()
{
    if (m_lastPng.isEmpty()) return;
    QPixmap pix(m_lastPng);
    if (pix.isNull()) return;
    QApplication::clipboard()->setPixmap(pix);
    m_status->setText(tr("Imagem copiada pra área de transferência."));
}
