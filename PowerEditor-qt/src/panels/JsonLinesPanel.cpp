#include "JsonLinesPanel.h"

#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSet>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

JsonLinesPanel::JsonLinesPanel(QWidget* parent) : QDockWidget(tr("JSONL"), parent)
{
    setObjectName(QStringLiteral("JsonLinesPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_openBtn = new QPushButton(tr("Abrir .jsonl…"), root);
    m_filter  = new QLineEdit(root); m_filter->setPlaceholderText(tr("Filtro (substring)…"));
    m_table   = new QTableWidget(root);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_detail  = new QPlainTextEdit(root); m_detail->setReadOnly(true); m_detail->setFont(mono);
    m_status  = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(m_openBtn);
    row->addWidget(m_filter, 1);

    auto* split = new QSplitter(Qt::Vertical, root);
    split->addWidget(m_table);
    split->addWidget(m_detail);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row);
    lay->addWidget(split, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_openBtn, &QPushButton::clicked, this, &JsonLinesPanel::onOpen);
    connect(m_filter,  &QLineEdit::textChanged, this, &JsonLinesPanel::onFilter);
    connect(m_table,   &QTableWidget::cellClicked, this, &JsonLinesPanel::onRowActivated);
}

void JsonLinesPanel::onOpen()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Abrir JSONL"),
        QString(), tr("JSON Lines (*.jsonl *.ndjson);;Todos (*)"));
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    m_rawLines.clear();
    while (!f.atEnd()) m_rawLines << QString::fromUtf8(f.readLine()).trimmed();
    f.close();

    // Coleta chaves do superset.
    QStringList keys;
    QSet<QString> seen;
    for (const QString& l : m_rawLines) {
        if (l.isEmpty()) continue;
        const QJsonDocument d = QJsonDocument::fromJson(l.toUtf8());
        if (!d.isObject()) continue;
        for (const QString& k : d.object().keys()) {
            if (!seen.contains(k)) { seen.insert(k); keys << k; }
        }
    }
    m_table->clear();
    m_table->setColumnCount(keys.size());
    m_table->setHorizontalHeaderLabels(keys);
    m_table->setRowCount(0);
    for (int i = 0; i < m_rawLines.size(); ++i) {
        const QString& l = m_rawLines[i];
        if (l.isEmpty()) continue;
        const QJsonDocument d = QJsonDocument::fromJson(l.toUtf8());
        if (!d.isObject()) continue;
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        const QJsonObject obj = d.object();
        for (int c = 0; c < keys.size(); ++c) {
            const QJsonValue v = obj.value(keys[c]);
            QString s;
            if (v.isString())      s = v.toString();
            else if (v.isDouble()) s = QString::number(v.toDouble());
            else if (v.isBool())   s = v.toBool() ? QStringLiteral("true") : QStringLiteral("false");
            else if (!v.isUndefined() && !v.isNull())
                s = QString::fromUtf8(QJsonDocument::fromVariant(v.toVariant()).toJson(QJsonDocument::Compact));
            auto* it = new QTableWidgetItem(s);
            it->setData(Qt::UserRole, i);
            m_table->setItem(row, c, it);
        }
    }
    m_status->setText(tr("%1 linha(s)").arg(m_table->rowCount()));
}

void JsonLinesPanel::onFilter(const QString& text)
{
    const QString needle = text.trimmed();
    for (int r = 0; r < m_table->rowCount(); ++r) {
        bool show = needle.isEmpty();
        if (!show) {
            for (int c = 0; c < m_table->columnCount() && !show; ++c) {
                if (auto* it = m_table->item(r, c))
                    if (it->text().contains(needle, Qt::CaseInsensitive)) show = true;
            }
        }
        m_table->setRowHidden(r, !show);
    }
}

void JsonLinesPanel::onRowActivated(int row, int)
{
    auto* it = m_table->item(row, 0);
    if (!it) return;
    const int idx = it->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= m_rawLines.size()) return;
    const QJsonDocument d = QJsonDocument::fromJson(m_rawLines[idx].toUtf8());
    m_detail->setPlainText(QString::fromUtf8(d.toJson(QJsonDocument::Indented)));
}
