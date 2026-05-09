#include "DocPreviewPanel.h"

#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QVBoxLayout>
#include <QWidget>

DocPreviewPanel::DocPreviewPanel(QWidget* parent) : QDockWidget(tr("Preview"), parent)
{
    setObjectName(QStringLiteral("DocPreviewPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);

    m_openBtn   = new QPushButton(tr("Abrir…"), root);
    m_renderBtn = new QPushButton(tr("Renderizar"), root);
    m_formatCombo = new QComboBox(root);
    m_formatCombo->addItems({ QStringLiteral("auto"), QStringLiteral("pdf"),
                              QStringLiteral("plantuml"), QStringLiteral("asciidoc"),
                              QStringLiteral("latex") });

    m_image = new QLabel();
    m_image->setAlignment(Qt::AlignCenter);
    m_image->setStyleSheet(QStringLiteral("QLabel { background: white; }"));
    m_scroll = new QScrollArea(root);
    m_scroll->setWidget(m_image);
    m_scroll->setWidgetResizable(false);

    m_html = new QPlainTextEdit(root);
    m_html->setReadOnly(true);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_html->setFont(mono);
    m_html->setVisible(false);

    m_status = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(m_openBtn);
    row->addWidget(new QLabel(tr("Formato:"), root));
    row->addWidget(m_formatCombo);
    row->addStretch(1);
    row->addWidget(m_renderBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row);
    lay->addWidget(m_scroll, 1);
    lay->addWidget(m_html, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_openBtn,   &QPushButton::clicked, this, &DocPreviewPanel::onOpen);
    connect(m_renderBtn, &QPushButton::clicked, this, &DocPreviewPanel::onRender);
}

QString DocPreviewPanel::detectFormat(const QString& path) const
{
    const QString ext = QFileInfo(path).suffix().toLower();
    if (ext == QStringLiteral("pdf"))                      return QStringLiteral("pdf");
    if (ext == QStringLiteral("puml") || ext == QStringLiteral("plantuml")) return QStringLiteral("plantuml");
    if (ext == QStringLiteral("adoc") || ext == QStringLiteral("asciidoc")) return QStringLiteral("asciidoc");
    if (ext == QStringLiteral("tex"))                      return QStringLiteral("latex");
    return QStringLiteral("auto");
}

bool DocPreviewPanel::openFile(const QString& path)
{
    m_path = path;
    QString fmt = m_formatCombo->currentText();
    if (fmt == QStringLiteral("auto")) fmt = detectFormat(path);
    m_format = fmt;
    setWindowTitle(tr("Preview — %1 (%2)").arg(QFileInfo(path).fileName(), m_format));
    onRender();
    return true;
}

void DocPreviewPanel::onOpen()
{
    const QString p = QFileDialog::getOpenFileName(this, tr("Abrir documento"),
        QString(), tr("Docs (*.pdf *.puml *.plantuml *.adoc *.asciidoc *.tex);;Todos (*)"));
    if (!p.isEmpty()) openFile(p);
}

void DocPreviewPanel::onRender()
{
    if (m_path.isEmpty()) { m_status->setText(tr("Sem arquivo.")); return; }
    QString fmt = m_formatCombo->currentText();
    if (fmt == QStringLiteral("auto")) fmt = detectFormat(m_path);
    m_format = fmt;
    m_status->setText(tr("Renderizando…"));
    m_html->setVisible(false);
    m_scroll->setVisible(true);

    QString err;
    bool ok = false;
    if      (fmt == QStringLiteral("pdf"))      ok = renderPdf(m_path, &err);
    else if (fmt == QStringLiteral("plantuml")) ok = renderPlantUml(m_path, &err);
    else if (fmt == QStringLiteral("asciidoc")) ok = renderAsciidoc(m_path, &err);
    else if (fmt == QStringLiteral("latex"))    ok = renderLatex(m_path, &err);
    else { err = tr("Formato desconhecido."); }

    m_status->setText(ok ? tr("OK") : tr("Erro: %1").arg(err));
}

bool DocPreviewPanel::renderPdf(const QString& src, QString* err)
{
    const QString exe = QStandardPaths::findExecutable(QStringLiteral("pdftoppm"));
    if (exe.isEmpty()) { *err = tr("pdftoppm não encontrado (instale poppler-utils)."); return false; }
    QTemporaryDir tmp; tmp.setAutoRemove(true);
    if (!tmp.isValid()) { *err = tr("temp dir falhou"); return false; }
    const QString prefix = tmp.path() + "/page";
    QProcess p;
    p.start(exe, { QStringLiteral("-r"), QStringLiteral("100"),
                   QStringLiteral("-f"), QStringLiteral("1"),
                   QStringLiteral("-l"), QStringLiteral("1"),
                   src, prefix });
    if (!p.waitForFinished(30000)) { p.kill(); *err = tr("timeout"); return false; }
    if (p.exitCode() != 0) { *err = QString::fromUtf8(p.readAllStandardError()); return false; }
    const QStringList files = QDir(tmp.path()).entryList({"page*.png"}, QDir::Files);
    if (files.isEmpty()) { *err = tr("Nenhuma página gerada."); return false; }
    QPixmap pix(tmp.path() + "/" + files.first());
    if (pix.isNull()) { *err = tr("Falha ao abrir página."); return false; }
    m_image->setPixmap(pix); m_image->resize(pix.size());
    return true;
}

bool DocPreviewPanel::renderPlantUml(const QString& src, QString* err)
{
    const QString exe = QStandardPaths::findExecutable(QStringLiteral("plantuml"));
    if (exe.isEmpty()) { *err = tr("plantuml não encontrado."); return false; }
    QTemporaryDir tmp; tmp.setAutoRemove(true);
    QProcess p;
    p.start(exe, { QStringLiteral("-tpng"), QStringLiteral("-o"), tmp.path(), src });
    if (!p.waitForFinished(30000)) { p.kill(); *err = tr("timeout"); return false; }
    if (p.exitCode() != 0) { *err = QString::fromUtf8(p.readAllStandardError()); return false; }
    const QStringList files = QDir(tmp.path()).entryList({"*.png"}, QDir::Files);
    if (files.isEmpty()) { *err = tr("Sem PNG gerado."); return false; }
    QPixmap pix(tmp.path() + "/" + files.first());
    m_image->setPixmap(pix); m_image->resize(pix.size());
    return true;
}

bool DocPreviewPanel::renderAsciidoc(const QString& src, QString* err)
{
    const QString exe = QStandardPaths::findExecutable(QStringLiteral("asciidoctor"));
    if (exe.isEmpty()) { *err = tr("asciidoctor não encontrado."); return false; }
    QTemporaryDir tmp; tmp.setAutoRemove(true);
    const QString out = tmp.path() + "/out.html";
    QProcess p;
    p.start(exe, { QStringLiteral("-b"), QStringLiteral("html5"),
                   QStringLiteral("-o"), out, src });
    if (!p.waitForFinished(30000)) { p.kill(); *err = tr("timeout"); return false; }
    if (p.exitCode() != 0) { *err = QString::fromUtf8(p.readAllStandardError()); return false; }
    QFile f(out);
    if (!f.open(QIODevice::ReadOnly)) { *err = tr("Falha ao ler HTML."); return false; }
    m_scroll->setVisible(false);
    m_html->setVisible(true);
    m_html->setPlainText(QString::fromUtf8(f.readAll()));
    f.close();
    return true;
}

bool DocPreviewPanel::renderLatex(const QString& src, QString* err)
{
    const QString exe = QStandardPaths::findExecutable(QStringLiteral("pdflatex"));
    if (exe.isEmpty()) { *err = tr("pdflatex não encontrado."); return false; }
    QTemporaryDir tmp; tmp.setAutoRemove(true);
    QProcess p; p.setWorkingDirectory(tmp.path());
    p.start(exe, { QStringLiteral("-interaction=nonstopmode"),
                   QStringLiteral("-output-directory"), tmp.path(), src });
    if (!p.waitForFinished(60000)) { p.kill(); *err = tr("timeout"); return false; }
    const QStringList pdfs = QDir(tmp.path()).entryList({"*.pdf"}, QDir::Files);
    if (pdfs.isEmpty()) { *err = QString::fromUtf8(p.readAllStandardError()); return false; }
    return renderPdf(tmp.path() + "/" + pdfs.first(), err);
}
