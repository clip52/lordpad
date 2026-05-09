#include "ColorPalettePanel.h"

#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QWidget>

#include <cmath>

ColorPalettePanel::ColorPalettePanel(QWidget* parent) : QDockWidget(tr("Cores"), parent)
{
    setObjectName(QStringLiteral("ColorPalettePanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);

    m_swatch = new QLabel(root);
    m_swatch->setMinimumHeight(60);
    m_swatch->setFrameStyle(QFrame::Box);
    m_hexEdit = new QLineEdit(root);
    m_hexEdit->setMaxLength(9);

    m_pickBtn   = new QPushButton(tr("Picker…"), root);
    m_copyBtn   = new QPushButton(tr("Copiar"), root);
    m_insertBtn = new QPushButton(tr("Inserir no editor"), root);
    m_addBtn    = new QPushButton(tr("+ paleta"), root);
    m_delBtn    = new QPushButton(tr("− paleta"), root);
    m_againstBtn= new QPushButton(tr("Contraste contra…"), root);

    m_palette = new QListWidget(root);
    m_palette->setViewMode(QListView::IconMode);
    m_palette->setMovement(QListView::Static);
    m_palette->setSpacing(4);
    m_palette->setFlow(QListView::LeftToRight);
    m_palette->setWrapping(true);
    m_palette->setResizeMode(QListView::Adjust);
    m_palette->setMinimumHeight(120);

    m_ratio = new QLabel(root);
    m_ratio->setStyleSheet(QStringLiteral("QLabel { font-weight: bold; }"));

    auto* row1 = new QHBoxLayout();
    row1->addWidget(m_swatch, 1);
    row1->addWidget(m_hexEdit);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(m_pickBtn);
    row2->addWidget(m_copyBtn);
    row2->addWidget(m_insertBtn);
    row2->addWidget(m_addBtn);
    row2->addWidget(m_delBtn);
    row2->addStretch(1);
    auto* row3 = new QHBoxLayout();
    row3->addWidget(m_againstBtn);
    row3->addWidget(m_ratio, 1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(new QLabel(tr("Paleta:"), root));
    lay->addWidget(m_palette, 1);
    lay->addLayout(row3);
    setWidget(root);

    connect(m_pickBtn,    &QPushButton::clicked, this, &ColorPalettePanel::onPickColor);
    connect(m_copyBtn,    &QPushButton::clicked, this, &ColorPalettePanel::onCopyHex);
    connect(m_insertBtn,  &QPushButton::clicked, this, &ColorPalettePanel::onInsertHex);
    connect(m_addBtn,     &QPushButton::clicked, this, &ColorPalettePanel::onAddToPalette);
    connect(m_delBtn,     &QPushButton::clicked, this, &ColorPalettePanel::onRemoveFromPalette);
    connect(m_againstBtn, &QPushButton::clicked, this, &ColorPalettePanel::onUpdateRatio);
    connect(m_palette,    &QListWidget::itemActivated, this, &ColorPalettePanel::onPaletteItemActivated);
    connect(m_hexEdit,    &QLineEdit::editingFinished, this, [this]() {
        const QColor c(m_hexEdit->text().trimmed());
        if (c.isValid()) { m_color = c; rerender(); }
    });

    loadPalette();
    rerender();
}

void ColorPalettePanel::onPickColor()
{
    QColor c = QColorDialog::getColor(m_color, this, tr("Cor"));
    if (c.isValid()) { m_color = c; rerender(); }
}

void ColorPalettePanel::onAddToPalette()
{
    auto* it = new QListWidgetItem();
    it->setSizeHint(QSize(28, 28));
    it->setBackground(QBrush(m_color));
    it->setData(Qt::UserRole, m_color.name());
    it->setToolTip(m_color.name());
    m_palette->addItem(it);
    persistPalette();
}

void ColorPalettePanel::onRemoveFromPalette()
{
    auto items = m_palette->selectedItems();
    for (auto* it : items) delete m_palette->takeItem(m_palette->row(it));
    persistPalette();
}

void ColorPalettePanel::onPaletteItemActivated(QListWidgetItem* it)
{
    if (!it) return;
    m_color = QColor(it->data(Qt::UserRole).toString());
    rerender();
}

void ColorPalettePanel::onUpdateRatio()
{
    QColor c = QColorDialog::getColor(m_against, this, tr("Cor de fundo p/ contraste"));
    if (c.isValid()) m_against = c;
    rerender();
}

void ColorPalettePanel::onCopyHex()
{
    QApplication::clipboard()->setText(m_color.name());
}

void ColorPalettePanel::onInsertHex()
{
    emit insertHexRequested(m_color.name());
}

void ColorPalettePanel::persistPalette() const
{
    QStringList colors;
    for (int i = 0; i < m_palette->count(); ++i)
        colors << m_palette->item(i)->data(Qt::UserRole).toString();
    QSettings s; s.setValue(QStringLiteral("ColorPalette/items"), colors);
}

void ColorPalettePanel::loadPalette()
{
    QSettings s;
    const QStringList colors = s.value(QStringLiteral("ColorPalette/items")).toStringList();
    for (const QString& c : colors) {
        auto* it = new QListWidgetItem();
        it->setSizeHint(QSize(28, 28));
        it->setBackground(QBrush(QColor(c)));
        it->setData(Qt::UserRole, c);
        it->setToolTip(c);
        m_palette->addItem(it);
    }
}

double ColorPalettePanel::luminance(const QColor& c)
{
    auto chan = [](double v) {
        v /= 255.0;
        return (v <= 0.03928) ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * chan(c.red()) + 0.7152 * chan(c.green()) + 0.0722 * chan(c.blue());
}

double ColorPalettePanel::contrast(const QColor& a, const QColor& b)
{
    const double la = luminance(a) + 0.05;
    const double lb = luminance(b) + 0.05;
    return (la > lb) ? la / lb : lb / la;
}

void ColorPalettePanel::rerender()
{
    m_swatch->setStyleSheet(QStringLiteral("QLabel { background: %1; }").arg(m_color.name()));
    m_hexEdit->setText(m_color.name());
    const double ratio = contrast(m_color, m_against);
    QString tier;
    if (ratio >= 7.0) tier = QStringLiteral("AAA");
    else if (ratio >= 4.5) tier = QStringLiteral("AA");
    else if (ratio >= 3.0) tier = QStringLiteral("AA Large");
    else                   tier = QStringLiteral("FALHA");
    m_ratio->setText(tr("Contra %1: %2  ·  ratio %3:1")
                         .arg(m_against.name(), tier).arg(ratio, 0, 'f', 2));
}
