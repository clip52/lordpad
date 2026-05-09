#include "MermaidPanel.h"

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QVBoxLayout>
#include <QWidget>

namespace {
constexpr const char* kExample =
    "graph TD\n"
    "    A[Início] --> B{Decisão?}\n"
    "    B -->|Sim| C[Caminho A]\n"
    "    B -->|Não| D[Caminho B]\n"
    "    C --> E[Fim]\n"
    "    D --> E\n";
}

MermaidPanel::MermaidPanel(QWidget* parent) : QDockWidget(tr("Mermaid"), parent)
{
    setObjectName(QStringLiteral("MermaidPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);

    m_source = new QPlainTextEdit(root);
    m_source->setPlaceholderText(tr("Cole o diagrama Mermaid aqui…"));
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(10);
    m_source->setFont(mono);

    m_renderBtn  = new QPushButton(tr("Renderizar"), root);
    m_saveBtn    = new QPushButton(tr("Salvar PNG…"), root);
    m_copyBtn    = new QPushButton(tr("Copiar imagem"), root);
    m_exampleBtn = new QPushButton(tr("Exemplo"), root);
    m_saveBtn->setEnabled(false);
    m_copyBtn->setEnabled(false);

    m_image = new QLabel();
    m_image->setAlignment(Qt::AlignCenter);
    m_image->setStyleSheet(QStringLiteral("QLabel { background: white; }"));

    m_scroll = new QScrollArea(root);
    m_scroll->setWidget(m_image);
    m_scroll->setWidgetResizable(true);
    m_scroll->setMinimumHeight(300);

    m_status = new QLabel(root);
    m_status->setWordWrap(true);

    auto* row = new QHBoxLayout();
    row->addWidget(m_renderBtn);
    row->addWidget(m_saveBtn);
    row->addWidget(m_copyBtn);
    row->addStretch(1);
    row->addWidget(m_exampleBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addWidget(m_source, 2);
    lay->addLayout(row);
    lay->addWidget(m_scroll, 3);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_renderBtn,  &QPushButton::clicked, this, &MermaidPanel::onRender);
    connect(m_saveBtn,    &QPushButton::clicked, this, &MermaidPanel::onSaveAs);
    connect(m_copyBtn,    &QPushButton::clicked, this, &MermaidPanel::onCopyImage);
    connect(m_exampleBtn, &QPushButton::clicked, this, &MermaidPanel::onLoadExample);

    if (QStandardPaths::findExecutable(QStringLiteral("mmdc")).isEmpty()) {
        m_status->setText(tr(
            "ℹ️  `mmdc` (mermaid-cli) não foi encontrado. Instale com:\n"
            "    npm install -g @mermaid-js/mermaid-cli\n"
            "ou via container, sem alterar o sistema:\n"
            "    npx -p @mermaid-js/mermaid-cli mmdc ..."));
    }
}

bool MermaidPanel::runMmdc(const QString& src, const QString& outPath, QString* outErr) const
{
    const QString exe = QStandardPaths::findExecutable(QStringLiteral("mmdc"));
    if (exe.isEmpty()) {
        if (outErr) *outErr = tr("mmdc não está instalado.");
        return false;
    }

    // mmdc reads from a file, not stdin. Use a per-call tmp .mmd file.
    QTemporaryFile inFile(QDir::tempPath() + "/mermaid_XXXXXX.mmd");
    inFile.setAutoRemove(true);
    if (!inFile.open()) {
        if (outErr) *outErr = tr("Falha ao criar arquivo temp."); return false;
    }
    inFile.write(src.toUtf8());
    inFile.flush();
    const QString inPath = inFile.fileName();

    QStringList args = {
        QStringLiteral("-i"), inPath,
        QStringLiteral("-o"), outPath,
        QStringLiteral("-b"), QStringLiteral("transparent"),
    };

    QProcess p;
    p.start(exe, args);
    if (!p.waitForStarted(2500)) {
        if (outErr) *outErr = tr("Não consegui iniciar mmdc.");
        return false;
    }
    // mmdc launches a headless Chromium internally; can take a while.
    if (!p.waitForFinished(60000)) { p.kill(); p.waitForFinished(500);
        if (outErr) *outErr = tr("mmdc demorou demais.");
        return false;
    }
    if (p.exitCode() != 0) {
        if (outErr) {
            *outErr = QString::fromUtf8(p.readAllStandardError()).trimmed();
            if (outErr->isEmpty())
                *outErr = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
        }
        return false;
    }
    return true;
}

void MermaidPanel::onRender()
{
    const QString src = m_source->toPlainText().trimmed();
    if (src.isEmpty()) { m_status->setText(tr("Diagrama vazio.")); return; }

    if (m_lastPng.isEmpty()) {
        QTemporaryFile tmp(QDir::tempPath() + "/mermaid_out_XXXXXX.png");
        tmp.setAutoRemove(false);
        if (!tmp.open()) { m_status->setText(tr("Falha ao criar arquivo temp.")); return; }
        m_lastPng = tmp.fileName();
        tmp.close();
    }

    m_status->setText(tr("Renderizando…"));
    QApplication::processEvents();

    QString err;
    if (!runMmdc(src, m_lastPng, &err)) {
        m_status->setText(tr("Erro: %1").arg(err));
        return;
    }
    QPixmap pix(m_lastPng);
    if (pix.isNull()) { m_status->setText(tr("PNG inválido.")); return; }
    m_image->setPixmap(pix);
    m_image->resize(pix.size());
    m_status->setText(tr("OK — %1×%2 px").arg(pix.width()).arg(pix.height()));
    m_saveBtn->setEnabled(true);
    m_copyBtn->setEnabled(true);
}

void MermaidPanel::onSaveAs()
{
    if (m_lastPng.isEmpty()) return;
    const QString dest = QFileDialog::getSaveFileName(this, tr("Salvar PNG"),
        QStringLiteral("diagrama.png"), tr("PNG (*.png)"));
    if (dest.isEmpty()) return;
    QFile::remove(dest);
    if (!QFile::copy(m_lastPng, dest)) {
        m_status->setText(tr("Falha ao salvar."));
        return;
    }
    m_status->setText(tr("Salvo em %1").arg(dest));
}

void MermaidPanel::onCopyImage()
{
    if (m_lastPng.isEmpty()) return;
    QPixmap pix(m_lastPng);
    if (pix.isNull()) return;
    QApplication::clipboard()->setPixmap(pix);
    m_status->setText(tr("Imagem copiada pra área de transferência."));
}

void MermaidPanel::onLoadExample()
{
    m_source->setPlainText(QString::fromUtf8(kExample));
}
