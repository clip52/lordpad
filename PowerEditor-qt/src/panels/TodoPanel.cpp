#include "TodoPanel.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QWidget>

TodoPanel::TodoPanel(QWidget* parent) : QDockWidget(tr("TODO"), parent)
{
    setObjectName(QStringLiteral("TodoPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    m_input  = new QLineEdit(root);
    m_input->setPlaceholderText(tr("Nova tarefa — Enter pra adicionar"));
    m_addBtn = new QPushButton(tr("Adicionar"), root);
    m_list   = new QListWidget(root);
    m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);

    m_delBtn       = new QPushButton(tr("Apagar selecionados"), root);
    m_clearDoneBtn = new QPushButton(tr("Limpar concluídos"), root);

    auto* row = new QHBoxLayout();
    row->addWidget(m_input, 1);
    row->addWidget(m_addBtn);

    auto* row2 = new QHBoxLayout();
    row2->addWidget(m_delBtn);
    row2->addWidget(m_clearDoneBtn);
    row2->addStretch(1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row);
    lay->addWidget(m_list, 1);
    lay->addLayout(row2);
    setWidget(root);

    connect(m_input,  &QLineEdit::returnPressed, this, &TodoPanel::onAdd);
    connect(m_addBtn, &QPushButton::clicked,     this, &TodoPanel::onAdd);
    connect(m_delBtn, &QPushButton::clicked,     this, &TodoPanel::onDelete);
    connect(m_clearDoneBtn, &QPushButton::clicked, this, &TodoPanel::onClearDone);
    connect(m_list,   &QListWidget::itemChanged, this, &TodoPanel::onItemChanged);

    load();
}

void TodoPanel::onAdd()
{
    const QString text = m_input->text().trimmed();
    if (text.isEmpty()) return;
    m_input->clear();
    m_suppressItemSignals = true;
    auto* it = new QListWidgetItem(text, m_list);
    it->setFlags(it->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
    it->setCheckState(Qt::Unchecked);
    m_suppressItemSignals = false;
    persist();
}

void TodoPanel::onItemChanged(QListWidgetItem* it)
{
    if (m_suppressItemSignals || !it) return;
    // Strikethrough done items so the visual matches the persisted state.
    QFont f = it->font();
    f.setStrikeOut(it->checkState() == Qt::Checked);
    it->setFont(f);
    persist();
}

void TodoPanel::onDelete()
{
    const auto sel = m_list->selectedItems();
    if (sel.isEmpty()) return;
    for (QListWidgetItem* it : sel) {
        delete m_list->takeItem(m_list->row(it));
    }
    persist();
}

void TodoPanel::onClearDone()
{
    for (int i = m_list->count() - 1; i >= 0; --i) {
        if (m_list->item(i)->checkState() == Qt::Checked)
            delete m_list->takeItem(i);
    }
    persist();
}

void TodoPanel::persist() const
{
    QStringList rows;
    for (int i = 0; i < m_list->count(); ++i) {
        auto* it = m_list->item(i);
        const QChar mark = (it->checkState() == Qt::Checked) ? QLatin1Char('x') : QLatin1Char(' ');
        rows.append(QStringLiteral("[%1] %2").arg(mark).arg(it->text()));
    }
    QSettings s;
    s.setValue(QStringLiteral("Todo/items"), rows);
}

void TodoPanel::load()
{
    m_suppressItemSignals = true;
    m_list->clear();
    QSettings s;
    const QStringList rows = s.value(QStringLiteral("Todo/items")).toStringList();
    for (const QString& r : rows) {
        // Tolerant parsing: accept "[x] text", "[X] text", "[ ] text", or
        // a plain unprefixed string (legacy).
        QString text = r;
        bool checked = false;
        if (text.startsWith(QStringLiteral("[x]")) || text.startsWith(QStringLiteral("[X]"))) {
            checked = true; text = text.mid(3).trimmed();
        } else if (text.startsWith(QStringLiteral("[ ]"))) {
            text = text.mid(3).trimmed();
        }
        auto* it = new QListWidgetItem(text, m_list);
        it->setFlags(it->flags() | Qt::ItemIsUserCheckable | Qt::ItemIsEditable);
        it->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
        if (checked) {
            QFont f = it->font(); f.setStrikeOut(true); it->setFont(f);
        }
    }
    m_suppressItemSignals = false;
}
