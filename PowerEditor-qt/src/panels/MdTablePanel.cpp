#include "MdTablePanel.h"

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include <ScintillaEdit.h>

MdTablePanel::MdTablePanel(QWidget* parent) : QDockWidget(tr("MD Table"), parent)
{
    setObjectName(QStringLiteral("MdTablePanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_table = new QTableWidget(root);
    m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_preview = new QPlainTextEdit(root);
    m_preview->setReadOnly(true); m_preview->setFont(mono);
    m_parseBtn  = new QPushButton(tr("Carregar do buffer"), root);
    m_addRowBtn = new QPushButton(tr("+ linha"), root);
    m_addColBtn = new QPushButton(tr("+ coluna"), root);
    m_writeBtn  = new QPushButton(tr("Escrever no buffer"), root);
    m_status    = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(m_parseBtn);
    row->addWidget(m_addRowBtn);
    row->addWidget(m_addColBtn);
    row->addStretch(1);
    row->addWidget(m_writeBtn);

    auto* split = new QSplitter(Qt::Vertical, root);
    split->addWidget(m_table); split->addWidget(m_preview);
    split->setStretchFactor(0, 3); split->setStretchFactor(1, 1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row);
    lay->addWidget(split, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_parseBtn,  &QPushButton::clicked, this, &MdTablePanel::onParse);
    connect(m_addRowBtn, &QPushButton::clicked, this, &MdTablePanel::onAddRow);
    connect(m_addColBtn, &QPushButton::clicked, this, &MdTablePanel::onAddCol);
    connect(m_writeBtn,  &QPushButton::clicked, this, &MdTablePanel::onWriteBack);
    connect(m_table,     &QTableWidget::itemChanged, this, [this](QTableWidgetItem*) {
        m_preview->setPlainText(renderMarkdown());
    });
}

void MdTablePanel::setActiveEditor(ScintillaEdit* editor) { m_editor = editor; }

void MdTablePanel::onParse()
{
    if (!m_editor) { m_status->setText(tr("Sem editor.")); return; }
    QByteArray bytes = m_editor->getText(m_editor->textLength() + 1);
    QString text = QString::fromUtf8(bytes);
    QStringList lines = text.split('\n');

    m_blockStart = m_blockEnd = -1;
    for (int i = 0; i < lines.size(); ++i) {
        if (lines[i].trimmed().startsWith('|') && lines[i].trimmed().endsWith('|')) {
            int j = i;
            while (j < lines.size() && lines[j].trimmed().startsWith('|')) ++j;
            if (j - i >= 2) { m_blockStart = i; m_blockEnd = j - 1; break; }
        }
    }
    if (m_blockStart < 0) { m_status->setText(tr("Nenhuma tabela MD encontrada.")); return; }

    auto split = [](const QString& line) -> QStringList {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith('|')) trimmed = trimmed.mid(1);
        if (trimmed.endsWith('|'))   trimmed.chop(1);
        QStringList parts = trimmed.split('|');
        for (QString& p : parts) p = p.trimmed();
        return parts;
    };

    const QStringList headers = split(lines[m_blockStart]);
    QList<QStringList> rows;
    for (int i = m_blockStart + 2; i <= m_blockEnd; ++i) rows.append(split(lines[i]));

    m_table->blockSignals(true);
    m_table->clear();
    m_table->setColumnCount(headers.size());
    m_table->setHorizontalHeaderLabels(headers);
    m_table->setRowCount(rows.size());
    for (int r = 0; r < rows.size(); ++r) {
        for (int c = 0; c < headers.size(); ++c) {
            const QString v = (c < rows[r].size()) ? rows[r][c] : QString();
            m_table->setItem(r, c, new QTableWidgetItem(v));
        }
    }
    m_table->blockSignals(false);
    m_preview->setPlainText(renderMarkdown());
    m_status->setText(tr("Tabela em linhas %1–%2").arg(m_blockStart + 1).arg(m_blockEnd + 1));
}

QString MdTablePanel::renderMarkdown() const
{
    const int cols = m_table->columnCount();
    const int rows = m_table->rowCount();
    if (cols == 0) return {};

    // Compute per-column width (max chars in header/body).
    QList<int> w; w.reserve(cols);
    QStringList headers;
    for (int c = 0; c < cols; ++c) {
        QString h = m_table->horizontalHeaderItem(c)
                       ? m_table->horizontalHeaderItem(c)->text() : QString();
        headers << h;
        int width = h.size();
        for (int r = 0; r < rows; ++r) {
            auto* it = m_table->item(r, c);
            if (it) width = qMax(width, it->text().size());
        }
        w << qMax(3, width);
    }

    auto pad = [](const QString& s, int n) {
        return s.leftJustified(n, ' ');
    };

    QStringList out;
    QStringList hdr;
    for (int c = 0; c < cols; ++c) hdr << pad(headers[c], w[c]);
    out << "| " + hdr.join(" | ") + " |";
    QStringList sep;
    for (int c = 0; c < cols; ++c) sep << QString(w[c], '-');
    out << "| " + sep.join(" | ") + " |";
    for (int r = 0; r < rows; ++r) {
        QStringList row;
        for (int c = 0; c < cols; ++c) {
            auto* it = m_table->item(r, c);
            row << pad(it ? it->text() : QString(), w[c]);
        }
        out << "| " + row.join(" | ") + " |";
    }
    return out.join('\n');
}

void MdTablePanel::onAddRow()
{
    const int r = m_table->rowCount();
    m_table->insertRow(r);
    for (int c = 0; c < m_table->columnCount(); ++c)
        m_table->setItem(r, c, new QTableWidgetItem(QString()));
    m_preview->setPlainText(renderMarkdown());
}
void MdTablePanel::onAddCol()
{
    const int c = m_table->columnCount();
    m_table->insertColumn(c);
    m_table->setHorizontalHeaderItem(c, new QTableWidgetItem(tr("col%1").arg(c + 1)));
    m_preview->setPlainText(renderMarkdown());
}

void MdTablePanel::onWriteBack()
{
    if (!m_editor || m_blockStart < 0) {
        m_status->setText(tr("Carregue uma tabela primeiro.")); return;
    }
    const QString md = renderMarkdown();
    const int from = static_cast<int>(m_editor->positionFromLine(m_blockStart));
    const int to   = static_cast<int>(m_editor->positionFromLine(m_blockEnd + 1));
    QByteArray bytes = md.toUtf8() + '\n';
    m_editor->beginUndoAction();
    m_editor->setTargetRange(from, to);
    m_editor->replaceTarget(bytes.size(), bytes.constData());
    m_editor->endUndoAction();
    m_status->setText(tr("Escrito no buffer."));
}
