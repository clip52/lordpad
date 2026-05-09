#include "GitLogPanel.h"

#include "../GitOps.h"

#include <QCheckBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

GitLogPanel::GitLogPanel(QWidget* parent)
    : QDockWidget(tr("Git Log"), parent)
{
    setObjectName(QStringLiteral("GitLogPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    auto* lay  = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);

    auto* row = new QHBoxLayout();
    m_onlyThis   = new QCheckBox(tr("Apenas este arquivo"), root);
    m_refreshBtn = new QPushButton(tr("Atualizar"), root);
    row->addWidget(m_onlyThis);
    row->addStretch(1);
    row->addWidget(m_refreshBtn);
    lay->addLayout(row);

    auto* split = new QSplitter(Qt::Vertical, root);

    m_table = new QTreeWidget(split);
    m_table->setHeaderLabels({ tr("SHA"), tr("Data"), tr("Autor"), tr("Mensagem") });
    m_table->setRootIsDecorated(false);
    m_table->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_table->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_table->header()->setSectionResizeMode(3, QHeaderView::Stretch);

    m_diff = new QPlainTextEdit(split);
    m_diff->setReadOnly(true);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(10);
    m_diff->setFont(mono);

    split->addWidget(m_table);
    split->addWidget(m_diff);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 2);
    lay->addWidget(split, 1);

    setWidget(root);

    connect(m_table,      &QTreeWidget::itemActivated, this, &GitLogPanel::onItemActivated);
    connect(m_refreshBtn, &QPushButton::clicked,        this, &GitLogPanel::refresh);
    connect(m_onlyThis,   &QCheckBox::toggled,          this, &GitLogPanel::onScopeToggled);
}

void GitLogPanel::setAnchorFile(const QString& filePath)
{
    if (m_anchor == filePath) return;
    m_anchor = filePath;
    refresh();
}

void GitLogPanel::onScopeToggled() { refresh(); }

void GitLogPanel::refresh()
{
    m_table->clear();
    m_diff->clear();
    if (m_anchor.isEmpty()) return;

    QList<GitOps::LogEntry> entries;
    QString err;
    const QString filter = (m_onlyThis && m_onlyThis->isChecked()) ? m_anchor : QString();
    if (!GitOps::log(m_anchor, /*max*/ 200, filter, entries, &err)) {
        auto* it = new QTreeWidgetItem(m_table);
        it->setText(3, tr("Erro: %1").arg(err));
        return;
    }
    for (const GitOps::LogEntry& e : entries) {
        auto* it = new QTreeWidgetItem(m_table);
        it->setText(0, e.sha);
        it->setText(1, e.date);
        it->setText(2, e.author);
        it->setText(3, e.subject);
        it->setData(0, Qt::UserRole, e.sha);
    }
}

void GitLogPanel::onItemActivated(QTreeWidgetItem* item, int)
{
    if (!item || m_anchor.isEmpty()) return;
    const QString sha = item->data(0, Qt::UserRole).toString();
    if (sha.isEmpty()) return;
    QString out, err;
    if (!GitOps::showDiff(m_anchor, sha, out, &err)) {
        m_diff->setPlainText(tr("Erro: %1").arg(err));
        return;
    }
    m_diff->setPlainText(out);
}
