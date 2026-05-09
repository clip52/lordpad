#include "PoEditorPanel.h"

#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>

PoEditorPanel::PoEditorPanel(QWidget* parent) : QDockWidget(tr("Tradução .po"), parent)
{
    setObjectName(QStringLiteral("PoEditorPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);

    m_openBtn = new QPushButton(tr("Abrir .po…"), root);
    m_saveBtn = new QPushButton(tr("Salvar"), root);

    auto* topRow = new QHBoxLayout();
    topRow->addWidget(m_openBtn);
    topRow->addWidget(m_saveBtn);
    topRow->addStretch(1);

    m_table = new QTableWidget(0, 3, root);
    m_table->setHorizontalHeaderLabels({ tr("Contexto"), tr("msgid"), tr("msgstr") });
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_msgid = new QPlainTextEdit(root);
    m_msgid->setReadOnly(true);
    m_msgstr = new QPlainTextEdit(root);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(10);
    m_msgid->setFont(mono);
    m_msgstr->setFont(mono);

    auto* split = new QSplitter(Qt::Vertical, root);
    auto* topWrap = new QWidget(root);
    auto* topLay = new QVBoxLayout(topWrap);
    topLay->setContentsMargins(0, 0, 0, 0);
    topLay->addWidget(new QLabel(tr("Entradas:"), root));
    topLay->addWidget(m_table, 1);
    auto* bottomWrap = new QWidget(root);
    auto* bottomLay  = new QVBoxLayout(bottomWrap);
    bottomLay->setContentsMargins(0, 0, 0, 0);
    bottomLay->addWidget(new QLabel(tr("msgid (origem):"), root));
    bottomLay->addWidget(m_msgid, 1);
    bottomLay->addWidget(new QLabel(tr("msgstr (tradução):"), root));
    bottomLay->addWidget(m_msgstr, 1);
    split->addWidget(topWrap);
    split->addWidget(bottomWrap);
    split->setStretchFactor(0, 2);
    split->setStretchFactor(1, 3);

    m_status = new QLabel(root);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(topRow);
    lay->addWidget(split, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_openBtn, &QPushButton::clicked, this, &PoEditorPanel::onOpen);
    connect(m_saveBtn, &QPushButton::clicked, this, &PoEditorPanel::onSave);
    connect(m_table,   &QTableWidget::itemSelectionChanged, this, &PoEditorPanel::onSelectionChanged);
    connect(m_msgstr,  &QPlainTextEdit::textChanged, this, &PoEditorPanel::onMsgstrChanged);
}

QString PoEditorPanel::unquote(const QString& quoted) const
{
    QString s = quoted.trimmed();
    if (s.size() >= 2 && s.startsWith('"') && s.endsWith('"'))
        s = s.mid(1, s.size() - 2);
    QString out;
    for (int i = 0; i < s.size(); ++i) {
        const QChar c = s[i];
        if (c == '\\' && i + 1 < s.size()) {
            const QChar n = s[i + 1];
            if      (n == 'n')  { out.append('\n'); ++i; }
            else if (n == 't')  { out.append('\t'); ++i; }
            else if (n == 'r')  { out.append('\r'); ++i; }
            else if (n == '"')  { out.append('"');  ++i; }
            else if (n == '\\') { out.append('\\'); ++i; }
            else                { out.append(c); }
        } else {
            out.append(c);
        }
    }
    return out;
}

QString PoEditorPanel::quote(const QString& raw) const
{
    QString out = QStringLiteral("\"");
    for (QChar c : raw) {
        if      (c == '\\') out.append(QStringLiteral("\\\\"));
        else if (c == '"')  out.append(QStringLiteral("\\\""));
        else if (c == '\n') out.append(QStringLiteral("\\n"));
        else if (c == '\t') out.append(QStringLiteral("\\t"));
        else if (c == '\r') out.append(QStringLiteral("\\r"));
        else                out.append(c);
    }
    out.append('"');
    return out;
}

bool PoEditorPanel::parsePoLines(const QStringList& lines)
{
    m_entries.clear();
    m_header.clear();
    Entry cur;
    QStringList comments;
    enum Field { None, Ctx, Id, Str };
    Field current = None;

    auto flush = [&]() {
        if (!cur.msgid.isEmpty() || !cur.msgstr.isEmpty() || !cur.ctx.isEmpty() || !comments.isEmpty()) {
            cur.comments = comments;
            m_entries.append(cur);
        }
        cur = Entry{};
        comments.clear();
        current = None;
    };

    for (const QString& raw : lines) {
        const QString line = raw;
        if (line.trimmed().isEmpty()) { flush(); continue; }
        if (line.startsWith('#')) { comments << line; current = None; continue; }
        if (line.startsWith(QStringLiteral("msgctxt"))) {
            cur.ctx   = unquote(line.mid(7));
            current   = Ctx;
        } else if (line.startsWith(QStringLiteral("msgid"))) {
            cur.msgid = unquote(line.mid(5));
            current   = Id;
        } else if (line.startsWith(QStringLiteral("msgstr"))) {
            cur.msgstr = unquote(line.mid(6));
            current    = Str;
        } else if (line.startsWith('"')) {
            // Continuation of the current field.
            if      (current == Ctx) cur.ctx    += unquote(line);
            else if (current == Id)  cur.msgid  += unquote(line);
            else if (current == Str) cur.msgstr += unquote(line);
        }
    }
    flush();

    // The first entry with empty msgid is the header — keep it as-is at top.
    if (!m_entries.isEmpty() && m_entries.first().msgid.isEmpty()) {
        m_header = m_entries.first().comments;
        // Header is kept inside m_entries[0] for round-trip; we still render it
        // as a special row in the table.
    }
    return true;
}

bool PoEditorPanel::openFile(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("PO"), tr("Não consegui abrir %1").arg(path));
        return false;
    }
    QTextStream in(&f);
    in.setEncoding(QStringConverter::Utf8);
    QStringList lines;
    while (!in.atEnd()) lines << in.readLine();
    f.close();

    if (!parsePoLines(lines)) {
        QMessageBox::warning(this, tr("PO"), tr("Erro de parse no .po."));
        return false;
    }
    m_filePath = path;
    setWindowTitle(tr("Tradução — %1").arg(QFileInfo(path).fileName()));
    rebuildTable();
    m_status->setText(tr("%1 entrada(s)").arg(m_entries.size()));
    return true;
}

bool PoEditorPanel::save()
{
    if (m_filePath.isEmpty()) {
        const QString p = QFileDialog::getSaveFileName(this, tr("Salvar .po"),
            QString(), tr("PO (*.po)"));
        if (p.isEmpty()) return false;
        m_filePath = p;
    }

    QFile f(m_filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("PO"), tr("Não consegui escrever %1").arg(m_filePath));
        return false;
    }
    QTextStream out(&f);
    out.setEncoding(QStringConverter::Utf8);
    for (int i = 0; i < m_entries.size(); ++i) {
        const Entry& e = m_entries[i];
        for (const QString& c : e.comments) out << c << "\n";
        if (!e.ctx.isEmpty())  out << "msgctxt " << quote(e.ctx)   << "\n";
        out << "msgid "  << quote(e.msgid)  << "\n";
        out << "msgstr " << quote(e.msgstr) << "\n";
        if (i + 1 < m_entries.size()) out << "\n";
    }
    f.close();
    m_status->setText(tr("Salvo: %1").arg(m_filePath));
    return true;
}

void PoEditorPanel::onOpen()
{
    const QString p = QFileDialog::getOpenFileName(this, tr("Abrir .po"),
        QString(), tr("PO (*.po)"));
    if (!p.isEmpty()) openFile(p);
}
void PoEditorPanel::onSave() { save(); }

void PoEditorPanel::rebuildTable()
{
    m_table->setRowCount(0);
    for (int i = 0; i < m_entries.size(); ++i) {
        const Entry& e = m_entries[i];
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        m_table->setItem(row, 0, new QTableWidgetItem(e.ctx));
        m_table->setItem(row, 1, new QTableWidgetItem(e.msgid));
        m_table->setItem(row, 2, new QTableWidgetItem(e.msgstr));
    }
    m_currentRow = -1;
}

void PoEditorPanel::onSelectionChanged()
{
    const int row = m_table->currentRow();
    if (row < 0 || row >= m_entries.size()) {
        m_msgid->clear(); m_msgstr->clear(); m_currentRow = -1;
        return;
    }
    m_currentRow = row;
    m_suppressMsgstrSignal = true;
    m_msgid->setPlainText(m_entries[row].msgid);
    m_msgstr->setPlainText(m_entries[row].msgstr);
    m_suppressMsgstrSignal = false;
}

void PoEditorPanel::onMsgstrChanged()
{
    if (m_suppressMsgstrSignal) return;
    if (m_currentRow < 0 || m_currentRow >= m_entries.size()) return;
    const QString text = m_msgstr->toPlainText();
    m_entries[m_currentRow].msgstr = text;
    if (auto* it = m_table->item(m_currentRow, 2)) it->setText(text);
}
