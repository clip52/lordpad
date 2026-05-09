#include "CodeClonesPanel.h"

#include <QHBoxLayout>
#include <QHash>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include <ScintillaEdit.h>

CodeClonesPanel::CodeClonesPanel(QWidget* parent) : QDockWidget(tr("Clones"), parent)
{
    setObjectName(QStringLiteral("CodeClonesPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);
    auto* root = new QWidget(this);
    m_window = new QSpinBox(root); m_window->setRange(2, 50); m_window->setValue(5);
    m_btn = new QPushButton(tr("Analisar"), root);
    m_tree = new QTreeWidget(root);
    m_tree->setHeaderLabels({ tr("Linhas"), tr("Ocorrências"), tr("Conteúdo") });
    m_tree->setRootIsDecorated(false);
    m_tree->header()->setStretchLastSection(true);
    m_status = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(new QLabel(tr("Janela (linhas):"), root));
    row->addWidget(m_window); row->addStretch(1); row->addWidget(m_btn);
    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row);
    lay->addWidget(m_tree, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_btn, &QPushButton::clicked, this, &CodeClonesPanel::onAnalyze);
    connect(m_tree, &QTreeWidget::itemActivated, this, &CodeClonesPanel::onItemActivated);
}

void CodeClonesPanel::onAnalyze()
{
    m_tree->clear();
    if (!m_editor) { m_status->setText(tr("Sem editor.")); return; }
    QByteArray bytes = m_editor->getText(m_editor->textLength() + 1);
    QStringList lines = QString::fromUtf8(bytes).split('\n');
    // Trim each line.
    for (QString& l : lines) l = l.trimmed();
    const int W = m_window->value();
    if (lines.size() < W * 2) { m_status->setText(tr("Buffer muito pequeno.")); return; }

    QHash<QString, QList<int>> groups;
    for (int i = 0; i + W <= lines.size(); ++i) {
        // Skip windows that are mostly empty or trivial.
        int blank = 0;
        for (int j = 0; j < W; ++j) if (lines[i + j].isEmpty()) ++blank;
        if (blank > W / 2) continue;
        QStringList window;
        for (int j = 0; j < W; ++j) window << lines[i + j];
        const QString key = window.join('\n');
        groups[key].append(i);
    }
    int dup = 0;
    for (auto it = groups.constBegin(); it != groups.constEnd(); ++it) {
        if (it.value().size() < 2) continue;
        ++dup;
        // Group header.
        QString preview = it.key().section('\n', 0, 0);
        if (preview.size() > 60) preview = preview.left(60) + QStringLiteral("…");
        for (int line0 : it.value()) {
            auto* row = new QTreeWidgetItem(m_tree);
            row->setText(0, QStringLiteral("%1–%2").arg(line0 + 1).arg(line0 + W));
            row->setText(1, QString::number(it.value().size()));
            row->setText(2, preview);
            row->setData(0, Qt::UserRole, line0 + 1);
        }
    }
    m_status->setText(tr("%1 grupo(s) duplicado(s)").arg(dup));
}

void CodeClonesPanel::onItemActivated(QTreeWidgetItem* it, int)
{
    if (!it) return;
    emit gotoLineRequested(it->data(0, Qt::UserRole).toInt());
}
