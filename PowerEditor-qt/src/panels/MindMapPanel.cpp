#include "MindMapPanel.h"

#include <QFileDialog>
#include <QFontDatabase>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QPainterPath>
#include <functional>
#include <QImage>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

namespace {
struct Node { QString text; int level; QList<Node*> children; Node* parent = nullptr; };
}

MindMapPanel::MindMapPanel(QWidget* parent) : QDockWidget(tr("Mind map"), parent)
{
    setObjectName(QStringLiteral("MindMapPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_source = new QPlainTextEdit(root); m_source->setFont(mono);
    m_source->setPlaceholderText(QStringLiteral("Raiz\n\tFilho 1\n\t\tNeto\n\tFilho 2\n"));
    m_source->setPlainText(QStringLiteral("Editor\n\tCore\n\t\tScintilla\n\t\tLexilla\n\tLSP\n\t\tcompletion\n\t\tdiagnostics\n\tGit\n"));
    m_renderBtn = new QPushButton(tr("Renderizar"), root);
    m_saveBtn   = new QPushButton(tr("Salvar PNG…"), root);
    m_scene = new QGraphicsScene(this);
    m_view  = new QGraphicsView(m_scene, root);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setBackgroundBrush(QColor("#1e1e1e"));

    auto* row = new QHBoxLayout();
    row->addWidget(m_renderBtn); row->addWidget(m_saveBtn); row->addStretch(1);
    auto* split = new QSplitter(Qt::Vertical, root);
    split->addWidget(m_source); split->addWidget(m_view);
    split->setStretchFactor(0, 1); split->setStretchFactor(1, 3);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row);
    lay->addWidget(split, 1);
    setWidget(root);

    connect(m_renderBtn, &QPushButton::clicked, this, &MindMapPanel::onRender);
    connect(m_saveBtn,   &QPushButton::clicked, this, &MindMapPanel::onSavePng);
    onRender();
}

void MindMapPanel::onRender()
{
    m_scene->clear();
    QStringList lines = m_source->toPlainText().split('\n');
    QList<Node*> all; QList<Node*> stack;
    Node* root = nullptr;
    for (const QString& raw : lines) {
        if (raw.trimmed().isEmpty()) continue;
        int lvl = 0;
        while (lvl < raw.size() && (raw[lvl] == '\t' || raw[lvl] == ' ')) ++lvl;
        QString text = raw.mid(lvl).trimmed();
        if (text.isEmpty()) continue;
        Node* n = new Node{text, lvl, {}, nullptr};
        all.append(n);
        if (!root) { root = n; stack = {n}; continue; }
        while (!stack.isEmpty() && stack.last()->level >= lvl) stack.removeLast();
        if (!stack.isEmpty()) {
            n->parent = stack.last();
            stack.last()->children.append(n);
        }
        stack.append(n);
    }
    if (!root) { qDeleteAll(all); return; }

    // Layout: BFS, position children to the right of parent, vertically distributed.
    QHash<Node*, QPointF> pos;
    pos[root] = QPointF(0, 0);
    int maxX = 0; int yCursor = 0;
    std::function<int(Node*, int, int)> placeY = [&](Node* n, int x, int y) -> int {
        pos[n] = QPointF(x, y);
        maxX = qMax(maxX, x);
        if (n->children.isEmpty()) return y + 40;
        int cy = y - (n->children.size() - 1) * 30;
        for (Node* c : n->children) {
            int next = placeY(c, x + 200, cy);
            cy = qMax(cy + 60, next);
        }
        return cy;
    };
    yCursor = placeY(root, 0, 0);

    // Edges first, then nodes on top.
    for (Node* n : all) {
        if (!n->parent) continue;
        const QPointF a = pos[n->parent], b = pos[n];
        QPainterPath path;
        path.moveTo(a + QPointF(160, 18));
        path.cubicTo(a + QPointF(180, 18), b + QPointF(0, 18), b + QPointF(20, 18));
        m_scene->addPath(path, QPen(QColor("#888"), 2));
    }
    for (Node* n : all) {
        const QPointF p = pos[n];
        QGraphicsRectItem* rect = m_scene->addRect(p.x(), p.y(), 160, 36,
            QPen(QColor("#3b82f6"), 2), QBrush(QColor("#243a55")));
        Q_UNUSED(rect);
        auto* lbl = m_scene->addText(n->text);
        lbl->setDefaultTextColor(Qt::white);
        lbl->setPos(p.x() + 8, p.y() + 6);
    }
    qDeleteAll(all);
    m_view->setSceneRect(m_scene->itemsBoundingRect().adjusted(-30, -30, 30, 30));
    m_view->fitInView(m_view->sceneRect(), Qt::KeepAspectRatio);
}

void MindMapPanel::onSavePng()
{
    const QString p = QFileDialog::getSaveFileName(this, tr("Salvar PNG"),
        QStringLiteral("mindmap.png"), tr("PNG (*.png)"));
    if (p.isEmpty()) return;
    const QRectF r = m_scene->itemsBoundingRect().adjusted(-30, -30, 30, 30);
    QImage img(r.size().toSize(), QImage::Format_ARGB32);
    img.fill(QColor("#1e1e1e"));
    QPainter pa(&img);
    pa.setRenderHint(QPainter::Antialiasing);
    m_scene->render(&pa, QRectF(0, 0, r.width(), r.height()), r);
    pa.end();
    img.save(p, "PNG");
}
