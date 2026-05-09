#include "ScreenshotPanel.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QProcess>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>

ScreenshotPanel::ScreenshotPanel(QWidget* parent) : QDockWidget(tr("Screenshot"), parent)
{
    setObjectName(QStringLiteral("ScreenshotPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);
    auto* root = new QWidget(this);
    m_mode = new QComboBox(root);
    m_mode->addItems({ tr("Tela inteira"), tr("Janela"), tr("Região") });
    m_delay = new QSpinBox(root); m_delay->setRange(0, 30); m_delay->setValue(0);
    m_captureBtn = new QPushButton(tr("Capturar"), root);
    m_saveBtn    = new QPushButton(tr("Salvar PNG…"), root);
    m_copyBtn    = new QPushButton(tr("Copiar"), root);
    m_saveBtn->setEnabled(false); m_copyBtn->setEnabled(false);
    m_thumb = new QLabel(root); m_thumb->setAlignment(Qt::AlignCenter);
    m_thumb->setStyleSheet(QStringLiteral("QLabel { background: #1e1e1e; min-height: 200px; }"));
    m_status = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(new QLabel(tr("Modo:"), root)); row->addWidget(m_mode);
    row->addWidget(new QLabel(tr("Delay (s):"), root)); row->addWidget(m_delay);
    row->addStretch(1);
    row->addWidget(m_captureBtn); row->addWidget(m_saveBtn); row->addWidget(m_copyBtn);
    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row);
    lay->addWidget(m_thumb, 1);
    lay->addWidget(m_status);
    setWidget(root);

    const QString tool = availableTool();
    if (tool.isEmpty()) m_status->setText(tr("Nenhuma ferramenta de screenshot encontrada (gnome-screenshot/spectacle/scrot/import)."));
    else                m_status->setText(tr("Usando: %1").arg(tool));

    connect(m_captureBtn, &QPushButton::clicked, this, &ScreenshotPanel::onCapture);
    connect(m_saveBtn,    &QPushButton::clicked, this, &ScreenshotPanel::onSaveAs);
    connect(m_copyBtn,    &QPushButton::clicked, this, &ScreenshotPanel::onCopy);
}

QString ScreenshotPanel::availableTool() const
{
    for (const QString& t : { QStringLiteral("gnome-screenshot"),
                               QStringLiteral("spectacle"),
                               QStringLiteral("scrot"),
                               QStringLiteral("import") })
        if (!QStandardPaths::findExecutable(t).isEmpty()) return t;
    return {};
}

void ScreenshotPanel::onCapture()
{
    const QString tool = availableTool();
    if (tool.isEmpty()) { m_status->setText(tr("Sem tool.")); return; }
    if (m_delay->value() > 0) {
        m_status->setText(tr("Aguardando %1s…").arg(m_delay->value()));
        QApplication::processEvents();
        QThread::sleep(static_cast<unsigned long>(m_delay->value()));
    }
    if (m_tmpPath.isEmpty()) {
        QTemporaryFile tmp(QDir::tempPath() + "/screenshot_XXXXXX.png");
        tmp.setAutoRemove(false);
        if (!tmp.open()) { m_status->setText(tr("Tmp falhou.")); return; }
        m_tmpPath = tmp.fileName(); tmp.close();
    }

    QStringList args;
    if (tool == QStringLiteral("gnome-screenshot")) {
        if (m_mode->currentIndex() == 1)      args << QStringLiteral("--window");
        else if (m_mode->currentIndex() == 2) args << QStringLiteral("--area");
        args << QStringLiteral("-f") << m_tmpPath;
    } else if (tool == QStringLiteral("spectacle")) {
        if (m_mode->currentIndex() == 1)      args << QStringLiteral("-a");
        else if (m_mode->currentIndex() == 2) args << QStringLiteral("-r");
        args << QStringLiteral("-b") << QStringLiteral("-n") << QStringLiteral("-o") << m_tmpPath;
    } else if (tool == QStringLiteral("scrot")) {
        if (m_mode->currentIndex() == 1)      args << QStringLiteral("-u");
        else if (m_mode->currentIndex() == 2) args << QStringLiteral("-s");
        args << m_tmpPath;
    } else { // ImageMagick import
        if (m_mode->currentIndex() == 0)      args << QStringLiteral("-window") << QStringLiteral("root");
        args << m_tmpPath;
    }

    QProcess p;
    p.start(tool, args);
    if (!p.waitForFinished(60000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }

    QPixmap pix(m_tmpPath);
    if (pix.isNull()) { m_status->setText(tr("PNG inválido.")); return; }
    m_thumb->setPixmap(pix.scaled(m_thumb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_saveBtn->setEnabled(true); m_copyBtn->setEnabled(true);
    m_status->setText(tr("OK — %1×%2").arg(pix.width()).arg(pix.height()));
}

void ScreenshotPanel::onSaveAs()
{
    if (m_tmpPath.isEmpty()) return;
    const QString p = QFileDialog::getSaveFileName(this, tr("Salvar"), QStringLiteral("screenshot.png"), tr("PNG (*.png)"));
    if (p.isEmpty()) return;
    QFile::remove(p); QFile::copy(m_tmpPath, p);
    m_status->setText(tr("Salvo em %1").arg(p));
}

void ScreenshotPanel::onCopy()
{
    if (m_tmpPath.isEmpty()) return;
    QPixmap pix(m_tmpPath);
    if (!pix.isNull()) QApplication::clipboard()->setPixmap(pix);
    m_status->setText(tr("Copiado pra área de transferência."));
}
