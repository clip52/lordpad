#include "ImageViewerPanel.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QImage>
#include <QImageReader>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

ImageViewerPanel::ImageViewerPanel(QWidget* parent) : QDockWidget(tr("Imagem"), parent)
{
    setObjectName(QStringLiteral("ImageViewerPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);

    m_openBtn    = new QPushButton(tr("Abrir…"), root);
    m_zoomInBtn  = new QToolButton(root);  m_zoomInBtn->setText(QStringLiteral("+"));
    m_zoomOutBtn = new QToolButton(root);  m_zoomOutBtn->setText(QStringLiteral("−"));
    m_fitBtn     = new QToolButton(root);  m_fitBtn->setText(tr("Ajustar"));
    m_actualBtn  = new QToolButton(root);  m_actualBtn->setText(tr("100%"));

    auto* row = new QHBoxLayout();
    row->addWidget(m_openBtn);
    row->addStretch(1);
    row->addWidget(m_zoomOutBtn);
    row->addWidget(m_zoomInBtn);
    row->addWidget(m_actualBtn);
    row->addWidget(m_fitBtn);

    m_imgLabel = new QLabel();
    m_imgLabel->setAlignment(Qt::AlignCenter);
    m_imgLabel->setStyleSheet(QStringLiteral("QLabel { background: #1e1e1e; }"));
    m_scroll = new QScrollArea(root);
    m_scroll->setWidget(m_imgLabel);
    m_scroll->setWidgetResizable(false);
    m_scroll->setMinimumHeight(200);

    m_status = new QLabel(root);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row);
    lay->addWidget(m_scroll, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_openBtn,    &QPushButton::clicked, this, &ImageViewerPanel::onOpen);
    connect(m_zoomInBtn,  &QToolButton::clicked, this, &ImageViewerPanel::onZoomIn);
    connect(m_zoomOutBtn, &QToolButton::clicked, this, &ImageViewerPanel::onZoomOut);
    connect(m_fitBtn,     &QToolButton::clicked, this, &ImageViewerPanel::onFit);
    connect(m_actualBtn,  &QToolButton::clicked, this, &ImageViewerPanel::onActualSize);
}

bool ImageViewerPanel::openFile(const QString& path)
{
    QImageReader r(path);
    r.setAutoTransform(true);
    QImage img = r.read();
    if (img.isNull()) {
        QMessageBox::warning(this, tr("Imagem"),
            tr("Não consegui ler %1: %2").arg(path, r.errorString()));
        return false;
    }
    delete m_image;
    m_image = new QImage(img);
    m_path = path;
    m_zoomPercent = 100;
    m_fitToWindow = false;
    setWindowTitle(tr("Imagem — %1").arg(QFileInfo(path).fileName()));
    rerender();
    return true;
}

void ImageViewerPanel::onOpen()
{
    const QString p = QFileDialog::getOpenFileName(this, tr("Abrir imagem"),
        QString(), tr("Imagens (*.png *.jpg *.jpeg *.gif *.bmp *.webp *.svg *.ico);;Todos (*)"));
    if (!p.isEmpty()) openFile(p);
}

void ImageViewerPanel::onZoomIn()
{
    m_fitToWindow = false;
    m_zoomPercent = qMin(800, m_zoomPercent + 25);
    rerender();
}
void ImageViewerPanel::onZoomOut()
{
    m_fitToWindow = false;
    m_zoomPercent = qMax(10, m_zoomPercent - 25);
    rerender();
}
void ImageViewerPanel::onFit()        { m_fitToWindow = true;  rerender(); }
void ImageViewerPanel::onActualSize() { m_fitToWindow = false; m_zoomPercent = 100; rerender(); }

void ImageViewerPanel::rerender()
{
    if (!m_image || m_image->isNull()) {
        m_imgLabel->clear();
        m_imgLabel->resize(0, 0);
        m_status->setText(tr("(sem imagem)"));
        return;
    }
    QPixmap pix = QPixmap::fromImage(*m_image);
    if (m_fitToWindow) {
        const QSize avail = m_scroll->viewport()->size();
        pix = pix.scaled(avail, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else {
        const QSize sz(m_image->width()  * m_zoomPercent / 100,
                       m_image->height() * m_zoomPercent / 100);
        pix = pix.scaled(sz, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    m_imgLabel->setPixmap(pix);
    m_imgLabel->resize(pix.size());
    m_status->setText(tr("%1×%2 px  ·  zoom %3%")
                          .arg(m_image->width()).arg(m_image->height()).arg(m_zoomPercent));
}
