#include "CsvChartPanel.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>
#include <QWidget>

#include <ScintillaEdit.h>

#include <algorithm>
#include <limits>

class ChartArea : public QWidget {
public:
    explicit ChartArea(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumHeight(220);
        setStyleSheet(QStringLiteral("background: #1e1e1e;"));
    }
    void setData(const QList<QPointF>& pts, const QString& xLabel, const QString& yLabel,
                 const QString& kind) {
        m_pts = pts; m_x = xLabel; m_y = yLabel; m_kind = kind;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.fillRect(rect(), QColor("#1e1e1e"));
        if (m_pts.size() < 2) {
            p.setPen(Qt::gray); p.drawText(rect(), Qt::AlignCenter, QStringLiteral("(sem dados)"));
            return;
        }
        // Find min/max.
        double minX = std::numeric_limits<double>::max(), maxX = -minX;
        double minY = minX, maxY = maxX;
        for (const QPointF& pt : m_pts) {
            minX = qMin(minX, pt.x()); maxX = qMax(maxX, pt.x());
            minY = qMin(minY, pt.y()); maxY = qMax(maxY, pt.y());
        }
        if (maxX == minX) maxX = minX + 1;
        if (maxY == minY) maxY = minY + 1;
        const QRect plot = rect().adjusted(50, 20, -20, -30);
        auto X = [&](double v) { return plot.left() + (v - minX) / (maxX - minX) * plot.width(); };
        auto Y = [&](double v) { return plot.bottom() - (v - minY) / (maxY - minY) * plot.height(); };

        // Axes.
        p.setPen(QColor("#555"));
        p.drawRect(plot);

        // Grid (horizontal lines).
        p.setPen(QPen(QColor("#333"), 1, Qt::DashLine));
        for (int i = 1; i < 5; ++i) {
            const double y = plot.bottom() - i * plot.height() / 5.0;
            p.drawLine(plot.left(), y, plot.right(), y);
        }

        // Plot.
        if (m_kind == QStringLiteral("Barra")) {
            const double bw = plot.width() / double(m_pts.size()) * 0.7;
            for (int i = 0; i < m_pts.size(); ++i) {
                const double x = X(m_pts[i].x());
                const double y = Y(m_pts[i].y());
                QRectF bar(x - bw / 2, y, bw, plot.bottom() - y);
                p.fillRect(bar, QColor("#3b82f6"));
            }
        } else {
            QPainterPath path;
            path.moveTo(X(m_pts[0].x()), Y(m_pts[0].y()));
            for (int i = 1; i < m_pts.size(); ++i)
                path.lineTo(X(m_pts[i].x()), Y(m_pts[i].y()));
            p.setPen(QPen(QColor("#3b82f6"), 2));
            p.drawPath(path);
        }

        // Labels (axis ranges).
        p.setPen(Qt::lightGray);
        p.drawText(QRect(0, plot.bottom() + 4, width(), 20), Qt::AlignHCenter, m_x);
        p.save();
        p.translate(8, plot.center().y());
        p.rotate(-90);
        p.drawText(QRect(-100, -10, 200, 20), Qt::AlignCenter, m_y);
        p.restore();
        p.drawText(QRect(plot.left() - 45, plot.top() - 4, 40, 18), Qt::AlignRight,
                   QString::number(maxY, 'g', 4));
        p.drawText(QRect(plot.left() - 45, plot.bottom() - 14, 40, 18), Qt::AlignRight,
                   QString::number(minY, 'g', 4));
    }

private:
    QList<QPointF> m_pts;
    QString m_x, m_y, m_kind;
};

CsvChartPanel::CsvChartPanel(QWidget* parent) : QDockWidget(tr("Chart"), parent)
{
    setObjectName(QStringLiteral("CsvChartPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    m_xCol = new QComboBox(root);
    m_yCol = new QComboBox(root);
    m_chartType = new QComboBox(root);
    m_chartType->addItems({ tr("Linha"), tr("Barra") });
    m_loadBtn = new QPushButton(tr("Recarregar"), root);
    m_plotBtn = new QPushButton(tr("Plot"), root);
    m_status  = new QLabel(root);
    m_area    = new ChartArea(root);

    auto* row = new QHBoxLayout();
    row->addWidget(new QLabel(tr("X:"), root)); row->addWidget(m_xCol);
    row->addWidget(new QLabel(tr("Y:"), root)); row->addWidget(m_yCol);
    row->addWidget(new QLabel(tr("Tipo:"), root)); row->addWidget(m_chartType);
    row->addStretch(1);
    row->addWidget(m_loadBtn); row->addWidget(m_plotBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row);
    lay->addWidget(m_area, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_loadBtn, &QPushButton::clicked, this, &CsvChartPanel::onLoad);
    connect(m_plotBtn, &QPushButton::clicked, this, &CsvChartPanel::onPlot);
}

void CsvChartPanel::setActiveEditor(ScintillaEdit* editor) { m_editor = editor; }

void CsvChartPanel::onLoad()
{
    if (!m_editor) { m_status->setText(tr("Sem editor.")); return; }
    QByteArray bytes = m_editor->getText(m_editor->textLength() + 1);
    QString text = QString::fromUtf8(bytes);
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    if (lines.isEmpty()) { m_status->setText(tr("Buffer vazio.")); return; }
    auto split = [](const QString& s) -> QStringList {
        // Naive CSV split — não trata quoting. Bom o suficiente para 99% dos casos.
        return s.split(QRegularExpression(QStringLiteral("[,;\\t]")));
    };
    m_headers = split(lines.first());
    m_rows.clear();
    for (int i = 1; i < lines.size(); ++i) m_rows.append(split(lines[i]));

    m_xCol->clear(); m_yCol->clear();
    for (int i = 0; i < m_headers.size(); ++i) {
        m_xCol->addItem(m_headers[i].trimmed(), i);
        m_yCol->addItem(m_headers[i].trimmed(), i);
    }
    if (m_yCol->count() > 1) m_yCol->setCurrentIndex(1);
    m_status->setText(tr("%1 colunas, %2 linhas").arg(m_headers.size()).arg(m_rows.size()));
}

void CsvChartPanel::onPlot()
{
    if (m_headers.isEmpty()) { m_status->setText(tr("Carregue o buffer primeiro.")); return; }
    const int xi = m_xCol->currentData().toInt();
    const int yi = m_yCol->currentData().toInt();
    QList<QPointF> pts;
    for (int r = 0; r < m_rows.size(); ++r) {
        const QStringList& cols = m_rows[r];
        if (xi >= cols.size() || yi >= cols.size()) continue;
        bool okX = false, okY = false;
        const double x = cols[xi].toDouble(&okX);
        const double y = cols[yi].toDouble(&okY);
        if (!okY) continue;
        pts.append(QPointF(okX ? x : double(r), y));
    }
    m_area->setData(pts,
                    m_xCol->currentText(),
                    m_yCol->currentText(),
                    m_chartType->currentText());
    m_status->setText(tr("%1 pontos").arg(pts.size()));
}
