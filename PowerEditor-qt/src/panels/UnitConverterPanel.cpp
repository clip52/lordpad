#include "UnitConverterPanel.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QWidget>

namespace {
struct Unit { const char* name; double toBase; };

// Each category has a "base" unit (1.0). Conversion is value*from.toBase/to.toBase.
// Temperature is a special case (offset) handled separately.
const Unit kLength[] = {
    {"mm",       0.001 }, {"cm",     0.01 },  {"m",      1.0 },
    {"km",       1000.0}, {"in",     0.0254}, {"ft",     0.3048},
    {"yd",       0.9144}, {"mi",     1609.344},
};
const Unit kMass[] = {
    {"mg",  0.000001}, {"g",  0.001}, {"kg", 1.0}, {"t", 1000.0},
    {"oz",  0.0283495}, {"lb", 0.453592},
};
const Unit kVolume[] = {
    {"ml", 0.001}, {"L", 1.0}, {"m³", 1000.0},
    {"tsp", 0.00492892}, {"tbsp", 0.0147868},
    {"cup", 0.236588}, {"pt", 0.473176}, {"qt", 0.946353}, {"gal", 3.78541},
};
const Unit kData[] = {
    {"B",  1.0}, {"KB", 1000.0}, {"MB", 1e6}, {"GB", 1e9}, {"TB", 1e12},
    {"KiB", 1024.0}, {"MiB", 1048576.0}, {"GiB", 1073741824.0}, {"TiB", 1099511627776.0},
};

double tempToCelsius(double v, const QString& from)
{
    if (from == QStringLiteral("°C")) return v;
    if (from == QStringLiteral("°F")) return (v - 32.0) * 5.0 / 9.0;
    if (from == QStringLiteral("K"))  return v - 273.15;
    return v;
}
double tempFromCelsius(double v, const QString& to)
{
    if (to == QStringLiteral("°C")) return v;
    if (to == QStringLiteral("°F")) return v * 9.0 / 5.0 + 32.0;
    if (to == QStringLiteral("K"))  return v + 273.15;
    return v;
}
}

UnitConverterPanel::UnitConverterPanel(QWidget* parent)
    : QDockWidget(tr("Conversor"), parent)
{
    setObjectName(QStringLiteral("UnitConverterPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);

    m_category = new QComboBox(root);
    m_category->addItems({ tr("Comprimento"), tr("Massa"), tr("Volume"),
                            tr("Temperatura"), tr("Dados") });
    m_fromUnit = new QComboBox(root);
    m_toUnit   = new QComboBox(root);
    m_input    = new QLineEdit(root);
    m_input->setPlaceholderText(tr("Valor"));
    m_result   = new QLabel(root);
    m_result->setStyleSheet(QStringLiteral("QLabel { font-size: 14pt; font-weight: bold; }"));
    m_result->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("Categoria:"), root));
    row1->addWidget(m_category, 1);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(m_input);
    row2->addWidget(m_fromUnit);
    row2->addWidget(new QLabel(QStringLiteral("→"), root));
    row2->addWidget(m_toUnit);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(m_result);
    lay->addStretch(1);
    setWidget(root);

    connect(m_category, &QComboBox::currentTextChanged, this, &UnitConverterPanel::onCategoryChanged);
    connect(m_input,    &QLineEdit::textChanged,        this, &UnitConverterPanel::onConvert);
    connect(m_fromUnit, &QComboBox::currentTextChanged, this, &UnitConverterPanel::onConvert);
    connect(m_toUnit,   &QComboBox::currentTextChanged, this, &UnitConverterPanel::onConvert);

    onCategoryChanged();
}

void UnitConverterPanel::onCategoryChanged()
{
    m_fromUnit->clear(); m_toUnit->clear();
    auto fill = [&](const Unit* arr, int n) {
        for (int i = 0; i < n; ++i) {
            m_fromUnit->addItem(QString::fromUtf8(arr[i].name));
            m_toUnit->addItem(QString::fromUtf8(arr[i].name));
        }
    };
    const QString cat = m_category->currentText();
    if (cat == tr("Comprimento"))      fill(kLength, sizeof(kLength)/sizeof(*kLength));
    else if (cat == tr("Massa"))       fill(kMass,   sizeof(kMass)/sizeof(*kMass));
    else if (cat == tr("Volume"))      fill(kVolume, sizeof(kVolume)/sizeof(*kVolume));
    else if (cat == tr("Dados"))       fill(kData,   sizeof(kData)/sizeof(*kData));
    else if (cat == tr("Temperatura")) {
        m_fromUnit->addItems({ QStringLiteral("°C"), QStringLiteral("°F"), QStringLiteral("K") });
        m_toUnit->  addItems({ QStringLiteral("°C"), QStringLiteral("°F"), QStringLiteral("K") });
    }
    if (m_fromUnit->count() > 1) m_toUnit->setCurrentIndex(1);
    onConvert();
}

void UnitConverterPanel::onConvert()
{
    bool ok = false;
    const double v = m_input->text().toDouble(&ok);
    if (!ok) { m_result->setText(QString()); return; }
    const QString cat = m_category->currentText();
    if (cat == tr("Temperatura")) {
        const double c = tempToCelsius(v, m_fromUnit->currentText());
        const double r = tempFromCelsius(c, m_toUnit->currentText());
        m_result->setText(QStringLiteral("= %1 %2").arg(r, 0, 'g', 10).arg(m_toUnit->currentText()));
        return;
    }
    auto find = [&](const QString& name, const Unit* arr, int n) -> const Unit* {
        for (int i = 0; i < n; ++i) if (QString::fromUtf8(arr[i].name) == name) return &arr[i];
        return nullptr;
    };
    const Unit* from = nullptr; const Unit* to = nullptr;
    if      (cat == tr("Comprimento")) {
        from = find(m_fromUnit->currentText(), kLength, sizeof(kLength)/sizeof(*kLength));
        to   = find(m_toUnit->currentText(),   kLength, sizeof(kLength)/sizeof(*kLength));
    } else if (cat == tr("Massa")) {
        from = find(m_fromUnit->currentText(), kMass,   sizeof(kMass)/sizeof(*kMass));
        to   = find(m_toUnit->currentText(),   kMass,   sizeof(kMass)/sizeof(*kMass));
    } else if (cat == tr("Volume")) {
        from = find(m_fromUnit->currentText(), kVolume, sizeof(kVolume)/sizeof(*kVolume));
        to   = find(m_toUnit->currentText(),   kVolume, sizeof(kVolume)/sizeof(*kVolume));
    } else if (cat == tr("Dados")) {
        from = find(m_fromUnit->currentText(), kData,   sizeof(kData)/sizeof(*kData));
        to   = find(m_toUnit->currentText(),   kData,   sizeof(kData)/sizeof(*kData));
    }
    if (!from || !to || to->toBase == 0) { m_result->setText(QString()); return; }
    const double r = v * from->toBase / to->toBase;
    m_result->setText(QStringLiteral("= %1 %2").arg(r, 0, 'g', 10).arg(m_toUnit->currentText()));
}
