#include "CallGraphPanel.h"

#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRegularExpression>
#include <QSet>
#include <QVBoxLayout>
#include <QWidget>

#include <ScintillaEdit.h>

CallGraphPanel::CallGraphPanel(QWidget* parent) : QDockWidget(tr("Call graph"), parent)
{
    setObjectName(QStringLiteral("CallGraphPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);
    auto* root = new QWidget(this);
    m_btn = new QPushButton(tr("Analisar buffer"), root);
    m_status = new QLabel(root);
    m_scene = new QGraphicsScene(this);
    m_view  = new QGraphicsView(m_scene, root);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setBackgroundBrush(QColor("#1e1e1e"));

    auto* row = new QHBoxLayout(); row->addWidget(m_btn); row->addStretch(1);
    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row);
    lay->addWidget(m_view, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_btn, &QPushButton::clicked, this, &CallGraphPanel::onAnalyze);
}

void CallGraphPanel::onAnalyze()
{
    m_scene->clear();
    if (!m_editor) { m_status->setText(tr("Sem editor.")); return; }
    QString text = QString::fromUtf8(m_editor->getText(m_editor->textLength() + 1));

    // Detect function definitions.
    static const QRegularExpression rxDef(
        QStringLiteral(R"((?:^|\n)\s*(?:def|fn|function|func)\s+(\w+)\s*\()"));
    static const QRegularExpression rxCDef(
        QStringLiteral(R"((?:^|\n)\s*[\w\s\*&:<>]+\s+(\w+)\s*\([^;]*\)\s*\{)"));
    QSet<QString> defs;
    auto add = [&](QRegularExpression& rx) {
        auto it = rx.globalMatch(text);
        while (it.hasNext()) defs.insert(it.next().captured(1));
    };
    add(const_cast<QRegularExpression&>(rxDef));
    add(const_cast<QRegularExpression&>(rxCDef));
    if (defs.isEmpty()) { m_status->setText(tr("Nenhuma função encontrada.")); return; }

    // For each def, find its body (next def or EOF) and count calls.
    struct Def { QString name; int from, to; };
    QList<Def> ranges;
    for (const QString& name : defs) {
        QRegularExpression rxFind(QStringLiteral(R"((?:def|fn|function|func)?\s*\b%1\s*\()").arg(QRegularExpression::escape(name)));
        // Simpler: locate first occurrence of "<name>(" with def-ish prefix.
        // Use the position of first ` <name>(` after "def " etc.
        int pos = text.indexOf(QStringLiteral("def ") + name + QStringLiteral("("));
        if (pos < 0) pos = text.indexOf(QStringLiteral("fn ") + name + QStringLiteral("("));
        if (pos < 0) pos = text.indexOf(QStringLiteral("function ") + name + QStringLiteral("("));
        if (pos < 0) pos = text.indexOf(QStringLiteral("func ") + name + QStringLiteral("("));
        if (pos < 0) pos = text.indexOf(QStringLiteral(" ") + name + QStringLiteral("("));
        if (pos < 0) continue;
        ranges.append({name, pos, static_cast<int>(text.size())});
    }
    // Sort by start, then trim each range to next start.
    std::sort(ranges.begin(), ranges.end(), [](const Def& a, const Def& b){ return a.from < b.from; });
    for (int i = 0; i + 1 < ranges.size(); ++i) ranges[i].to = ranges[i + 1].from;

    QHash<QString, QSet<QString>> calls;
    for (const Def& d : ranges) {
        const QString body = text.mid(d.from, d.to - d.from);
        for (const QString& target : defs) {
            if (target == d.name) continue;
            if (body.contains(QRegularExpression(QStringLiteral(R"(\b%1\s*\()").arg(QRegularExpression::escape(target)))))
                calls[d.name].insert(target);
        }
    }

    // Layout: simple grid.
    QHash<QString, QPointF> pos;
    int i = 0;
    for (const QString& name : defs) {
        const int col = i % 4;
        const int rowi = i / 4;
        pos[name] = QPointF(col * 200, rowi * 100);
        ++i;
    }
    // Edges.
    for (auto it = calls.constBegin(); it != calls.constEnd(); ++it) {
        for (const QString& target : it.value()) {
            if (!pos.contains(it.key()) || !pos.contains(target)) continue;
            const QPointF a = pos[it.key()] + QPointF(80, 18);
            const QPointF b = pos[target]  + QPointF(80, 18);
            QPainterPath p; p.moveTo(a);
            p.cubicTo(a + QPointF(0, 50), b + QPointF(0, -50), b);
            m_scene->addPath(p, QPen(QColor("#888"), 2));
        }
    }
    // Nodes.
    for (auto it = pos.constBegin(); it != pos.constEnd(); ++it) {
        m_scene->addRect(it.value().x(), it.value().y(), 160, 36,
                         QPen(QColor("#10b981"), 2), QBrush(QColor("#1f3d2e")));
        auto* lbl = m_scene->addText(it.key());
        lbl->setDefaultTextColor(Qt::white);
        lbl->setPos(it.value().x() + 8, it.value().y() + 6);
    }
    m_view->setSceneRect(m_scene->itemsBoundingRect().adjusted(-20, -20, 20, 20));
    m_view->fitInView(m_view->sceneRect(), Qt::KeepAspectRatio);
    m_status->setText(tr("%1 funções, %2 chamadas").arg(defs.size())
                          .arg([&](){int n=0; for (auto it=calls.constBegin();it!=calls.constEnd();++it) n+=it.value().size(); return n;}()));
}
