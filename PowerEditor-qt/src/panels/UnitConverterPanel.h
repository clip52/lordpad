#pragma once

#include <QDockWidget>

class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QPushButton;
class QLabel;

// UnitConverterPanel (M18) — comprimento / massa / temperatura / volume / dado.
class UnitConverterPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit UnitConverterPanel(QWidget* parent = nullptr);

private slots:
    void onCategoryChanged();
    void onConvert();

private:
    QComboBox*      m_category = nullptr;
    QComboBox*      m_fromUnit = nullptr;
    QComboBox*      m_toUnit   = nullptr;
    QLineEdit*      m_input    = nullptr;
    QLabel*         m_result   = nullptr;
};
