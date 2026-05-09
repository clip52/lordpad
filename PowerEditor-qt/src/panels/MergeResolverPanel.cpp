#include "MergeResolverPanel.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include <ScintillaEdit.h>

namespace {
constexpr const char* kStartMarker = "<<<<<<<";
constexpr const char* kBaseMarker  = "|||||||";
constexpr const char* kSepMarker   = "=======";
constexpr const char* kEndMarker   = ">>>>>>>";
}

MergeResolverPanel::MergeResolverPanel(QWidget* parent)
    : QDockWidget(tr("Conflitos de Merge"), parent)
{
    setObjectName(QStringLiteral("MergeResolverPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);

    m_status = new QLabel(tr("Pressione Atualizar pra escanear o buffer."), root);
    m_status->setWordWrap(true);

    m_tree = new QTreeWidget(root);
    m_tree->setHeaderLabels({ tr("Conflito"), tr("Linha"), tr("Resumo") });
    m_tree->setRootIsDecorated(false);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::Stretch);

    m_btnRescan = new QPushButton(tr("Atualizar"), root);
    m_btnOurs   = new QPushButton(tr("Aceitar Nosso"), root);
    m_btnTheirs = new QPushButton(tr("Aceitar Deles"), root);
    m_btnBoth   = new QPushButton(tr("Manter Ambos"), root);
    for (auto* b : { m_btnOurs, m_btnTheirs, m_btnBoth }) b->setEnabled(false);

    auto* row = new QHBoxLayout();
    row->addWidget(m_btnRescan);
    row->addStretch(1);
    row->addWidget(m_btnOurs);
    row->addWidget(m_btnTheirs);
    row->addWidget(m_btnBoth);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addWidget(m_status);
    lay->addWidget(m_tree, 1);
    lay->addLayout(row);
    setWidget(root);

    connect(m_btnRescan, &QPushButton::clicked, this, &MergeResolverPanel::onScan);
    connect(m_btnOurs,   &QPushButton::clicked, this, &MergeResolverPanel::onAcceptOurs);
    connect(m_btnTheirs, &QPushButton::clicked, this, &MergeResolverPanel::onAcceptTheirs);
    connect(m_btnBoth,   &QPushButton::clicked, this, &MergeResolverPanel::onKeepBoth);
    connect(m_tree, &QTreeWidget::itemActivated, this, &MergeResolverPanel::onItemActivated);
    connect(m_tree, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* it, QTreeWidgetItem*) {
        const bool hasSel = it != nullptr;
        m_btnOurs  ->setEnabled(hasSel);
        m_btnTheirs->setEnabled(hasSel);
        m_btnBoth  ->setEnabled(hasSel);
    });
}

void MergeResolverPanel::setActiveEditor(ScintillaEdit* editor)
{
    m_editor = editor;
    m_conflicts.clear();
    m_tree->clear();
    if (editor) m_status->setText(tr("Pressione Atualizar pra escanear."));
    else        m_status->setText(tr("Sem editor."));
}

QList<MergeResolverPanel::Conflict> MergeResolverPanel::findConflicts(ScintillaEdit* sci) const
{
    QList<Conflict> out;
    if (!sci) return out;
    const int lineCount = static_cast<int>(sci->lineCount());

    int i = 0;
    while (i < lineCount) {
        QByteArray rawLine = sci->getLine(i);
        QString line = QString::fromUtf8(rawLine);
        if (!line.startsWith(QString::fromUtf8(kStartMarker))) { ++i; continue; }

        Conflict c;
        c.startMarkerLine = i;
        int j = i + 1;
        QStringList ours;
        QStringList theirs;
        QStringList* dest = &ours;

        while (j < lineCount) {
            QByteArray rawJ = sci->getLine(j);
            QString lineJ = QString::fromUtf8(rawJ);
            if (lineJ.startsWith(QString::fromUtf8(kBaseMarker))) {
                // diff3 base separator — `ours` was actually the working tree
                // side (we already collected). From here until ======= is the
                // BASE, which we discard for accept-ours/theirs purposes.
                c.baseSeparatorLine = j;
                dest = nullptr;   // skip until =======
            } else if (lineJ.startsWith(QString::fromUtf8(kSepMarker))) {
                c.separatorLine = j;
                dest = &theirs;
            } else if (lineJ.startsWith(QString::fromUtf8(kEndMarker))) {
                c.endMarkerLine = j;
                break;
            } else if (dest) {
                // Strip the trailing newline so QString::join stays tidy.
                if (lineJ.endsWith('\n')) lineJ.chop(1);
                if (lineJ.endsWith('\r')) lineJ.chop(1);
                dest->append(lineJ);
            }
            ++j;
        }

        if (c.endMarkerLine > c.startMarkerLine && c.separatorLine > c.startMarkerLine) {
            c.ours   = ours.join('\n');
            c.theirs = theirs.join('\n');
            out.append(c);
            i = c.endMarkerLine + 1;
        } else {
            // Unclosed marker — bail and continue scanning past it.
            ++i;
        }
    }
    return out;
}

void MergeResolverPanel::onScan()
{
    m_conflicts.clear();
    m_tree->clear();
    if (!m_editor) { m_status->setText(tr("Sem editor.")); return; }

    m_conflicts = findConflicts(m_editor.data());
    if (m_conflicts.isEmpty()) {
        m_status->setText(tr("Nenhum conflito encontrado."));
        return;
    }
    for (int idx = 0; idx < m_conflicts.size(); ++idx) {
        const Conflict& c = m_conflicts[idx];
        QString summary = c.ours;
        if (summary.size() > 80) summary = summary.left(80) + QStringLiteral("…");
        QString lineLabel = QString::number(c.startMarkerLine + 1)
                          + QStringLiteral("..")
                          + QString::number(c.endMarkerLine + 1);
        auto* it = new QTreeWidgetItem(m_tree);
        it->setText(0, QString::number(idx + 1));
        it->setText(1, lineLabel);
        it->setText(2, summary);
        it->setData(0, Qt::UserRole, idx);
    }
    m_status->setText(tr("%1 conflito(s).").arg(m_conflicts.size()));
}

MergeResolverPanel::Conflict* MergeResolverPanel::selectedConflict()
{
    auto* it = m_tree->currentItem();
    if (!it) return nullptr;
    const int idx = it->data(0, Qt::UserRole).toInt();
    if (idx < 0 || idx >= m_conflicts.size()) return nullptr;
    return &m_conflicts[idx];
}

void MergeResolverPanel::applyResolution(int conflictIndex, const QString& replacement)
{
    if (!m_editor || conflictIndex < 0 || conflictIndex >= m_conflicts.size()) return;
    const Conflict& c = m_conflicts[conflictIndex];

    // Compute byte offsets that span the entire conflict (including the
    // closing marker line plus its line ending).
    auto* sci = m_editor.data();
    const int startByte = static_cast<int>(sci->positionFromLine(c.startMarkerLine));
    int endByte = static_cast<int>(sci->positionFromLine(c.endMarkerLine + 1));
    if (endByte <= startByte) {
        // last-line conflict without trailing newline
        endByte = static_cast<int>(sci->length());
    }

    QByteArray repl = replacement.toUtf8();
    // Ensure the replacement ends with a newline so subsequent content stays
    // on its own line (mirrors the trailing newline we just consumed).
    if (!repl.endsWith('\n')) repl += '\n';

    sci->beginUndoAction();
    sci->setTargetRange(startByte, endByte);
    sci->replaceTarget(repl.size(), repl.constData());
    sci->endUndoAction();

    // Re-scan; line numbers shifted, indices invalidated.
    onScan();
    emit resolutionApplied(m_conflicts.size());
}

void MergeResolverPanel::onAcceptOurs()
{
    auto* c = selectedConflict();
    if (!c) return;
    const int idx = m_tree->currentItem()->data(0, Qt::UserRole).toInt();
    applyResolution(idx, c->ours);
}

void MergeResolverPanel::onAcceptTheirs()
{
    auto* c = selectedConflict();
    if (!c) return;
    const int idx = m_tree->currentItem()->data(0, Qt::UserRole).toInt();
    applyResolution(idx, c->theirs);
}

void MergeResolverPanel::onKeepBoth()
{
    auto* c = selectedConflict();
    if (!c) return;
    const int idx = m_tree->currentItem()->data(0, Qt::UserRole).toInt();
    QString joined = c->ours;
    if (!joined.isEmpty() && !joined.endsWith('\n')) joined += '\n';
    joined += c->theirs;
    applyResolution(idx, joined);
}

void MergeResolverPanel::onItemActivated(QTreeWidgetItem* it, int)
{
    if (!it || !m_editor) return;
    const int idx = it->data(0, Qt::UserRole).toInt();
    if (idx < 0 || idx >= m_conflicts.size()) return;
    m_editor->gotoLine(m_conflicts[idx].startMarkerLine);
    m_editor->scrollCaret();
}
