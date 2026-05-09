#pragma once

#include <QDockWidget>
#include <QString>
#include <QStringList>
#include <QList>
#include <QPointF>

class QComboBox;
class QPushButton;
class QLabel;
class QWidget;
class ScintillaEdit;

class ChartArea;

// CsvChartPanel (M33) — plot rápido de colunas do buffer CSV ativo.
//   * detecta header da primeira linha
//   * combo X column / Y columns + tipo (linha / barra)
//   * desenho via QPainter num widget custom (sem Qt6Charts dep)
class CsvChartPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit CsvChartPanel(QWidget* parent = nullptr);

    void setActiveEditor(ScintillaEdit* editor);

private slots:
    void onLoad();
    void onPlot();

private:
    ScintillaEdit*  m_editor = nullptr;
    QStringList     m_headers;
    QList<QStringList> m_rows;

    QComboBox*      m_xCol = nullptr;
    QComboBox*      m_yCol = nullptr;
    QComboBox*      m_chartType = nullptr;
    QPushButton*    m_loadBtn = nullptr;
    QPushButton*    m_plotBtn = nullptr;
    QLabel*         m_status = nullptr;
    ChartArea*      m_area = nullptr;
};
