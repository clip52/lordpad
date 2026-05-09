#include "ClipboardHistoryPanel.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace { constexpr int kMaxItems = 50; }

ClipboardHistoryPanel::ClipboardHistoryPanel(QWidget* parent)
    : QDockWidget(tr("Clipboard"), parent)
{
    setObjectName(QStringLiteral("ClipboardHistoryPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    m_list = new QListWidget(root);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_clearBtn = new QPushButton(tr("Limpar"), root);

    auto* row = new QHBoxLayout();
    row->addStretch(1); row->addWidget(m_clearBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addWidget(m_list, 1);
    lay->addLayout(row);
    setWidget(root);

    connect(QApplication::clipboard(), &QClipboard::dataChanged,
            this, &ClipboardHistoryPanel::onClipboardChanged);
    connect(m_list, &QListWidget::itemActivated, this, &ClipboardHistoryPanel::onItemActivated);
    connect(m_clearBtn, &QPushButton::clicked, this, &ClipboardHistoryPanel::onClear);

    onClipboardChanged();   // seed if there's already something on the clipboard
}

void ClipboardHistoryPanel::onClipboardChanged()
{
    const QString text = QApplication::clipboard()->text();
    if (text.isEmpty()) return;
    if (m_list->count() > 0 && m_list->item(0)->text() == text) return; // skip duplicate of head
    auto* it = new QListWidgetItem(text);
    it->setToolTip(text);
    m_list->insertItem(0, it);
    while (m_list->count() > kMaxItems) delete m_list->takeItem(m_list->count() - 1);
}

void ClipboardHistoryPanel::onItemActivated(QListWidgetItem* it)
{
    if (!it) return;
    QApplication::clipboard()->setText(it->text());
    emit insertTextRequested(it->text());
}

void ClipboardHistoryPanel::onClear() { m_list->clear(); }
