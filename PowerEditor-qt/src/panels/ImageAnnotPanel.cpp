#include "ImageAnnotPanel.h"

#include <QEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

ImageAnnotPanel::ImageAnnotPanel(QWidget* parent) : QDockWidget(tr("Anotar imagem"), parent)
{
    setObjectName(QStringLiteral("ImageAnnotPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);

    m_view = new QLabel();
    m_view->setStyleSheet(QStringLiteral("QLabel { background: #1e1e1e; }"));
    m_view->setAlignment(Qt::AlignCenter);
    m_view->setMouseTracking(true);
    m_view->installEventFilter(this);

    m_scroll = new QScrollArea(root);
    m_scroll->setWidget(m_view);
    m_scroll->setWidgetResizable(false);
    m_scroll->setMinimumHeight(300);

    m_list = new QListWidget(root);
    m_list->setMaximumWidth(220);

    m_openBtn  = new QPushButton(tr("Abrir…"), root);
    m_saveBtn  = new QPushButton(tr("Salvar PNG…"), root);
    m_delBtn   = new QPushButton(tr("Apagar pino"), root);
    m_clearBtn = new QPushButton(tr("Limpar"), root);

    m_status = new QLabel(root);

    auto* split = new QSplitter(Qt::Horizontal, root);
    split->addWidget(m_scroll);
    split->addWidget(m_list);
    split->setStretchFactor(0, 4); split->setStretchFactor(1, 1);

    auto* row = new QHBoxLayout();
    row->addWidget(m_openBtn); row->addWidget(m_saveBtn);
    row->addStretch(1);
    row->addWidget(m_delBtn); row->addWidget(m_clearBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row);
    lay->addWidget(split, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_openBtn,  &QPushButton::clicked, this, &ImageAnnotPanel::onOpen);
    connect(m_saveBtn,  &QPushButton::clicked, this, &ImageAnnotPanel::onSaveAs);
    connect(m_delBtn,   &QPushButton::clicked, this, &ImageAnnotPanel::onDeleteSelected);
    connect(m_clearBtn, &QPushButton::clicked, this, &ImageAnnotPanel::onClear);
    connect(m_list,     &QListWidget::itemChanged, this, [this](QListWidgetItem*) { onRowEdited(); });
}

bool ImageAnnotPanel::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_view && event->type() == QEvent::MouseButtonPress) {
        auto* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::LeftButton && !m_image.isNull()) {
            // Mapeia a posição clicada do label (que pode estar com a imagem em
            // tamanho real) pra coordenada da imagem. m_view->pixmap exibe sempre
            // em escala 1:1 aqui (não usamos zoom).
            const QPoint p = me->pos();
            if (p.x() < 0 || p.y() < 0 || p.x() >= m_image.width() || p.y() >= m_image.height())
                return false;
            Pin pin; pin.pos = p; pin.label = QString::number(m_pins.size() + 1);
            m_pins.append(pin);
            auto* it = new QListWidgetItem(QStringLiteral("%1: %2").arg(pin.label).arg(QStringLiteral("(clique pra editar)")), m_list);
            it->setFlags(it->flags() | Qt::ItemIsEditable);
            it->setData(Qt::UserRole, m_pins.size() - 1);
            rerender();
            return true;
        }
    }
    return QDockWidget::eventFilter(watched, event);
}

void ImageAnnotPanel::onOpen()
{
    const QString p = QFileDialog::getOpenFileName(this, tr("Abrir imagem"),
        QString(), tr("Imagens (*.png *.jpg *.jpeg *.bmp *.gif)"));
    if (p.isEmpty()) return;
    QImage img(p);
    if (img.isNull()) { m_status->setText(tr("Falha ao abrir.")); return; }
    m_image = img; m_path = p;
    m_pins.clear(); m_list->clear();
    rerender();
    m_status->setText(tr("%1×%2 px").arg(img.width()).arg(img.height()));
}

void ImageAnnotPanel::onSaveAs()
{
    if (m_image.isNull()) return;
    const QString p = QFileDialog::getSaveFileName(this, tr("Salvar PNG"),
        QStringLiteral("annotated.png"), tr("PNG (*.png)"));
    if (p.isEmpty()) return;
    // Renderiza off-screen e salva.
    QImage out = m_image.copy();
    QPainter painter(&out);
    painter.setRenderHint(QPainter::Antialiasing);
    QFont f = painter.font(); f.setBold(true); f.setPointSize(12); painter.setFont(f);
    for (int i = 0; i < m_pins.size(); ++i) {
        const auto& pin = m_pins[i];
        painter.setBrush(QColor("#dc2626"));
        painter.setPen(QPen(Qt::white, 2));
        painter.drawEllipse(pin.pos, 12, 12);
        painter.setPen(Qt::white);
        painter.drawText(pin.pos.x() - 4, pin.pos.y() + 5, QString::number(i + 1));
        // Label drop-shadow.
        painter.setPen(QPen(Qt::black));
        painter.drawText(pin.pos.x() + 16, pin.pos.y() + 5, pin.label);
    }
    painter.end();
    if (out.save(p, "PNG")) m_status->setText(tr("Salvo em %1").arg(p));
    else                    m_status->setText(tr("Falha ao salvar."));
}

void ImageAnnotPanel::onDeleteSelected()
{
    auto items = m_list->selectedItems();
    if (items.isEmpty()) return;
    QList<int> idx;
    for (auto* it : items) idx << it->data(Qt::UserRole).toInt();
    std::sort(idx.begin(), idx.end(), std::greater<int>());
    for (int i : idx) if (i >= 0 && i < m_pins.size()) m_pins.removeAt(i);
    // Rebuild list.
    m_list->clear();
    for (int i = 0; i < m_pins.size(); ++i) {
        auto* it = new QListWidgetItem(QStringLiteral("%1: %2").arg(i + 1).arg(m_pins[i].label), m_list);
        it->setFlags(it->flags() | Qt::ItemIsEditable);
        it->setData(Qt::UserRole, i);
    }
    rerender();
}

void ImageAnnotPanel::onClear()
{
    m_pins.clear(); m_list->clear(); rerender();
}

void ImageAnnotPanel::onRowEdited()
{
    for (int i = 0; i < m_list->count() && i < m_pins.size(); ++i) {
        QString text = m_list->item(i)->text();
        // Strip "N: " prefix if present.
        const int colon = text.indexOf(':');
        if (colon > 0) text = text.mid(colon + 1).trimmed();
        m_pins[i].label = text;
    }
    rerender();
}

void ImageAnnotPanel::rerender()
{
    if (m_image.isNull()) { m_view->clear(); m_view->resize(0, 0); return; }
    QImage canvas = m_image.copy();
    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing);
    for (int i = 0; i < m_pins.size(); ++i) {
        const auto& pin = m_pins[i];
        painter.setBrush(QColor("#dc2626"));
        painter.setPen(QPen(Qt::white, 2));
        painter.drawEllipse(pin.pos, 10, 10);
        painter.setPen(Qt::white);
        painter.drawText(pin.pos.x() - 3, pin.pos.y() + 4, QString::number(i + 1));
    }
    painter.end();
    QPixmap pix = QPixmap::fromImage(canvas);
    m_view->setPixmap(pix);
    m_view->resize(pix.size());
}
