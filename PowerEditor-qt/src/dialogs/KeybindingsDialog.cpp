#include "KeybindingsDialog.h"

#include "../KeybindingsManager.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

KeybindingsDialog::KeybindingsDialog(KeybindingsManager* manager, QWidget* parent)
    : QDialog(parent), m_manager(manager)
{
    setWindowTitle(tr("Atalhos de teclado"));
    resize(720, 480);

    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText(tr("Filtrar por ação…"));

    m_table = new QTableWidget(this);
    m_table->setColumnCount(3);
    m_table->setHorizontalHeaderLabels({ tr("Ação"), tr("Padrão"), tr("Customizado") });
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(false);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::DoubleClicked
                            | QAbstractItemView::SelectedClicked
                            | QAbstractItemView::EditKeyPressed);

    m_resetThis = new QPushButton(tr("Resetar selecionado"), this);
    m_resetAll  = new QPushButton(tr("Resetar tudo"), this);

    auto* row = new QHBoxLayout();
    row->addWidget(m_resetThis);
    row->addWidget(m_resetAll);
    row->addStretch(1);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::accept);

    auto* lay = new QVBoxLayout(this);
    lay->addWidget(new QLabel(tr("Clique duplo na coluna Customizado e tecle a nova combinação."), this));
    lay->addWidget(m_filter);
    lay->addWidget(m_table, 1);
    lay->addLayout(row);
    lay->addWidget(bb);

    connect(m_filter,    &QLineEdit::textChanged, this, &KeybindingsDialog::onFilterChanged);
    connect(m_table,     &QTableWidget::cellChanged, this, &KeybindingsDialog::onCellChanged);
    connect(m_resetAll,  &QPushButton::clicked, this, &KeybindingsDialog::onResetAll);
    connect(m_resetThis, &QPushButton::clicked, this, &KeybindingsDialog::onResetSelected);

    rebuild();
}

void KeybindingsDialog::rebuild()
{
    if (!m_manager) return;
    m_suppressEdits = true;
    m_table->setRowCount(0);
    const auto entries = m_manager->entries();
    for (const auto& e : entries) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);

        auto* nameItem = new QTableWidgetItem(e.key);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, 0, nameItem);

        auto* defItem = new QTableWidgetItem(e.defaultShortcut.toString(QKeySequence::NativeText));
        defItem->setFlags(defItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, 1, defItem);

        auto* curItem = new QTableWidgetItem(e.currentShortcut.toString(QKeySequence::NativeText));
        curItem->setData(Qt::UserRole, e.key);
        m_table->setItem(row, 2, curItem);
    }
    m_suppressEdits = false;
}

void KeybindingsDialog::onFilterChanged(const QString& text)
{
    const QString needle = text.trimmed();
    for (int r = 0; r < m_table->rowCount(); ++r) {
        const QString name = m_table->item(r, 0)->text();
        m_table->setRowHidden(r, !needle.isEmpty()
                                 && !name.contains(needle, Qt::CaseInsensitive));
    }
}

void KeybindingsDialog::onCellChanged(int row, int column)
{
    if (m_suppressEdits || !m_manager) return;
    if (!isShortcutItem(column)) return;
    auto* item = m_table->item(row, column);
    if (!item) return;
    const QString key = item->data(Qt::UserRole).toString();
    QKeySequence seq = QKeySequence::fromString(item->text(), QKeySequence::NativeText);
    if (seq.isEmpty()) {
        seq = QKeySequence::fromString(item->text(), QKeySequence::PortableText);
    }
    if (!m_manager->setOverride(key, seq)) return;
    // Round-trip through the manager so the cell shows the canonical text.
    const auto entries = m_manager->entries();
    for (const auto& e : entries) {
        if (e.key == key) {
            m_suppressEdits = true;
            item->setText(e.currentShortcut.toString(QKeySequence::NativeText));
            m_suppressEdits = false;
            break;
        }
    }
}

void KeybindingsDialog::onResetSelected()
{
    if (!m_manager) return;
    const int row = m_table->currentRow();
    if (row < 0) return;
    auto* item = m_table->item(row, 2);
    if (!item) return;
    const QString key = item->data(Qt::UserRole).toString();
    m_manager->setOverride(key, QKeySequence());
    rebuild();
}

void KeybindingsDialog::onResetAll()
{
    if (!m_manager) return;
    m_manager->resetAll();
    rebuild();
}
