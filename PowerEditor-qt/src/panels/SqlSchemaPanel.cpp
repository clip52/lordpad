#include "SqlSchemaPanel.h"

#include <QBrush>
#include <QFileDialog>
#include <QFontDatabase>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QRegularExpression>
#include <QVBoxLayout>
#include <QWidget>

#include <ScintillaEdit.h>

SqlSchemaPanel::SqlSchemaPanel(QWidget* parent) : QDockWidget(tr("SQL Schema"), parent)
{
    setObjectName(QStringLiteral("SqlSchemaPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    m_scene = new QGraphicsScene(this);
    m_view  = new QGraphicsView(m_scene, root);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);

    m_parseBtn   = new QPushButton(tr("Parse + desenhar"), root);
    m_savePngBtn = new QPushButton(tr("Salvar PNG"), root);
    m_status     = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(m_parseBtn);
    row->addWidget(m_savePngBtn);
    row->addStretch(1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row);
    lay->addWidget(m_view, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_parseBtn,   &QPushButton::clicked, this, &SqlSchemaPanel::onParseAndDraw);
    connect(m_savePngBtn, &QPushButton::clicked, this, &SqlSchemaPanel::onSavePng);
}

void SqlSchemaPanel::setActiveEditor(ScintillaEdit* editor) { m_editor = editor; }

QList<SqlSchemaPanel::Table> SqlSchemaPanel::parse(const QString& sql) const
{
    QList<Table> out;
    QRegularExpression rx(QStringLiteral(R"RX(CREATE\s+TABLE\s+(?:IF\s+NOT\s+EXISTS\s+)?[`"']?(\w+)[`"']?\s*\(([^;]*?)\)\s*;)RX"),
                          QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
    auto it = rx.globalMatch(sql);
    while (it.hasNext()) {
        auto m = it.next();
        Table t;
        t.name = m.captured(1);
        const QString body = m.captured(2);
        // split on commas at top level
        int depth = 0; QString cur;
        QStringList parts;
        for (QChar ch : body) {
            if (ch == '(') depth++;
            else if (ch == ')') depth--;
            if (ch == ',' && depth == 0) { parts << cur.trimmed(); cur.clear(); }
            else cur += ch;
        }
        if (!cur.trimmed().isEmpty()) parts << cur.trimmed();

        QRegularExpression rxFk(QStringLiteral(R"RX(FOREIGN\s+KEY\s*\(\s*[`"']?(\w+)[`"']?\s*\)\s+REFERENCES\s+[`"']?(\w+)[`"']?\s*\(\s*[`"']?(\w+)[`"']?\s*\))RX"),
                                QRegularExpression::CaseInsensitiveOption);
        for (const QString& p : parts) {
            const QString upper = p.toUpper();
            auto fk = rxFk.match(p);
            if (fk.hasMatch()) {
                t.foreignKeys << QStringLiteral("%1 → %2.%3")
                                     .arg(fk.captured(1), fk.captured(2), fk.captured(3));
                continue;
            }
            if (upper.startsWith("PRIMARY KEY") || upper.startsWith("UNIQUE")
             || upper.startsWith("KEY ") || upper.startsWith("INDEX ")
             || upper.startsWith("CONSTRAINT")) continue;
            // first token is column name, rest type info
            QRegularExpression rxCol(QStringLiteral(R"RX(^\s*[`"']?(\w+)[`"']?\s+([^,\n]+))RX"));
            auto cm = rxCol.match(p);
            if (cm.hasMatch()) {
                QString type = cm.captured(2).trimmed();
                if (type.size() > 30) type = type.left(30) + "…";
                t.columns << QStringLiteral("%1 %2").arg(cm.captured(1), type);
            }
        }
        out << t;
    }
    return out;
}

void SqlSchemaPanel::draw(const QList<Table>& tables)
{
    m_scene->clear();
    if (tables.isEmpty()) { m_status->setText(tr("Nenhuma CREATE TABLE detectada.")); return; }

    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    QFont bold = mono; bold.setBold(true);

    QHash<QString, QGraphicsRectItem*> nodes;
    int x = 0, y = 0, maxRowH = 0;
    int colCount = 0;
    for (const Table& t : tables) {
        QGraphicsTextItem header;
        header.setFont(bold); header.setPlainText(t.name);
        const qreal headerH = header.boundingRect().height();
        qreal width = header.boundingRect().width() + 24;
        qreal h = headerH + 8;
        for (const QString& c : t.columns) {
            QGraphicsTextItem ti; ti.setFont(mono); ti.setPlainText(c);
            width = qMax(width, ti.boundingRect().width() + 24);
            h += ti.boundingRect().height();
        }
        for (const QString& fk : t.foreignKeys) {
            QGraphicsTextItem ti; ti.setFont(mono); ti.setPlainText("FK: " + fk);
            width = qMax(width, ti.boundingRect().width() + 24);
            h += ti.boundingRect().height();
        }

        auto* rect = m_scene->addRect(x, y, width, h, QPen(Qt::darkGray), QBrush(QColor(40, 50, 70)));
        auto* hdr = m_scene->addText(t.name, bold);
        hdr->setDefaultTextColor(QColor(220, 220, 240));
        hdr->setPos(x + 6, y + 4);

        qreal cy = y + headerH + 8;
        for (const QString& c : t.columns) {
            auto* ti = m_scene->addText(c, mono);
            ti->setDefaultTextColor(QColor(200, 220, 200));
            ti->setPos(x + 6, cy);
            cy += ti->boundingRect().height();
        }
        for (const QString& fk : t.foreignKeys) {
            auto* ti = m_scene->addText("FK: " + fk, mono);
            ti->setDefaultTextColor(QColor(255, 200, 120));
            ti->setPos(x + 6, cy);
            cy += ti->boundingRect().height();
        }

        nodes.insert(t.name, rect);
        maxRowH = qMax(maxRowH, int(h));

        x += int(width) + 40;
        ++colCount;
        if (colCount % 3 == 0) { x = 0; y += maxRowH + 50; maxRowH = 0; }
    }

    // FK lines
    QPen pen(QColor(255, 200, 120)); pen.setWidth(2);
    for (const Table& t : tables) {
        if (!nodes.contains(t.name)) continue;
        const QRectF a = nodes[t.name]->rect();
        for (const QString& fk : t.foreignKeys) {
            const QString ref = fk.section(QStringLiteral(" → "), 1).section('.', 0, 0);
            if (!nodes.contains(ref)) continue;
            const QRectF b = nodes[ref]->rect();
            m_scene->addLine(QLineF(a.center(), b.center()), pen);
        }
    }

    m_view->fitInView(m_scene->itemsBoundingRect().adjusted(-20, -20, 20, 20), Qt::KeepAspectRatio);
    m_status->setText(tr("%1 tabelas").arg(tables.size()));
}

void SqlSchemaPanel::onParseAndDraw()
{
    if (!m_editor) { m_status->setText(tr("Sem editor.")); return; }
    QByteArray bytes = m_editor->getText(m_editor->textLength() + 1);
    draw(parse(QString::fromUtf8(bytes)));
}

void SqlSchemaPanel::onSavePng()
{
    const QString p = QFileDialog::getSaveFileName(this, tr("Salvar"), QStringLiteral("schema.png"), tr("PNG (*.png)"));
    if (p.isEmpty()) return;
    const QRectF r = m_scene->itemsBoundingRect().adjusted(-20, -20, 20, 20);
    QImage img(int(r.width()), int(r.height()), QImage::Format_ARGB32_Premultiplied);
    img.fill(QColor(20, 20, 30));
    QPainter pr(&img);
    pr.setRenderHint(QPainter::Antialiasing);
    m_scene->render(&pr, QRectF(0, 0, r.width(), r.height()), r);
    pr.end();
    img.save(p, "PNG");
    m_status->setText(tr("Salvo em %1").arg(p));
}
