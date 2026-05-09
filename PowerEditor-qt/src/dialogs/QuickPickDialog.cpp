#include "QuickPickDialog.h"

#include <QDialogButtonBox>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

QuickPickDialog::QuickPickDialog(QWidget* parent, const QString& title,
                                 const QList<Item>& items)
    : QDialog(parent), m_items(items)
{
    setWindowTitle(title);
    resize(620, 420);

    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText(tr("Filtrar…"));
    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(bb, &QDialogButtonBox::accepted, this, &QuickPickDialog::onAccept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* lay = new QVBoxLayout(this);
    lay->addWidget(m_filter);
    lay->addWidget(m_list, 1);
    lay->addWidget(bb);

    connect(m_filter, &QLineEdit::textChanged, this, &QuickPickDialog::onFilterChanged);
    connect(m_list,   &QListWidget::itemActivated, this,
            [this](QListWidgetItem*) { onAccept(); });

    // Start with everything visible.
    onFilterChanged(QString());
    m_filter->setFocus();

}

void QuickPickDialog::setLiveQueryMode(bool on) { m_liveQuery = on; }
QString QuickPickDialog::currentQuery() const   { return m_filter ? m_filter->text() : QString(); }

void QuickPickDialog::replaceItems(const QList<Item>& items)
{
    m_items = items;
    onFilterChanged(m_filter ? m_filter->text() : QString());
}

void QuickPickDialog::onFilterChanged(const QString& text)
{
    if (m_liveQuery) emit queryChanged(text);

    m_list->clear();
    const QString needle = text.trimmed();
    for (int i = 0; i < m_items.size(); ++i) {
        const Item& it = m_items[i];
        if (!m_liveQuery && !needle.isEmpty()) {
            if (!it.label.contains(needle, Qt::CaseInsensitive) &&
                !it.subtitle.contains(needle, Qt::CaseInsensitive)) continue;
        }
        QString display = it.label;
        if (!it.subtitle.isEmpty()) display += QStringLiteral("   ─  ") + it.subtitle;
        auto* row = new QListWidgetItem(display);
        row->setData(Qt::UserRole, i);
        m_list->addItem(row);
    }
    if (m_list->count() > 0) m_list->setCurrentRow(0);
}

void QuickPickDialog::onAccept()
{
    auto* row = m_list->currentItem();
    if (!row) { reject(); return; }
    const int idx = row->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= m_items.size()) { reject(); return; }
    m_picked = m_items[idx].data;
    accept();
}
