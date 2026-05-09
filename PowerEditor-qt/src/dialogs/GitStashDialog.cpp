#include "GitStashDialog.h"

#include "../GitOps.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

GitStashDialog::GitStashDialog(const QString& anchorFilePath, QWidget* parent)
    : QDialog(parent), m_anchor(anchorFilePath)
{
    setWindowTitle(tr("Stash"));
    resize(560, 460);

    m_status = new QLabel(this);

    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);

    m_message  = new QLineEdit(this);
    m_message->setPlaceholderText(tr("Mensagem do stash (opcional)"));
    m_untracked = new QCheckBox(tr("Incluir não-rastreados"), this);

    m_pushBtn  = new QPushButton(tr("Stash (push)"), this);
    m_applyBtn = new QPushButton(tr("Apply"), this);
    m_popBtn   = new QPushButton(tr("Pop"), this);
    m_dropBtn  = new QPushButton(tr("Drop"), this);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(m_message, 1);
    row1->addWidget(m_untracked);
    row1->addWidget(m_pushBtn);

    auto* row2 = new QHBoxLayout();
    row2->addStretch(1);
    row2->addWidget(m_applyBtn);
    row2->addWidget(m_popBtn);
    row2->addWidget(m_dropBtn);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::accept);

    auto* lay = new QVBoxLayout(this);
    lay->addWidget(m_status);
    lay->addWidget(m_list, 1);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(bb);

    connect(m_pushBtn,  &QPushButton::clicked, this, &GitStashDialog::onPush);
    connect(m_applyBtn, &QPushButton::clicked, this, &GitStashDialog::onApply);
    connect(m_popBtn,   &QPushButton::clicked, this, &GitStashDialog::onPop);
    connect(m_dropBtn,  &QPushButton::clicked, this, &GitStashDialog::onDrop);

    rebuild();
}

void GitStashDialog::rebuild()
{
    m_list->clear();
    QList<GitOps::StashEntry> entries;
    QString err;
    if (!GitOps::stashList(m_anchor, entries, &err)) {
        m_status->setText(tr("Erro: %1").arg(err));
        return;
    }
    if (entries.isEmpty()) m_status->setText(tr("Sem stashes."));
    else                   m_status->setText(tr("%1 stash(es)").arg(entries.size()));
    for (const GitOps::StashEntry& e : entries) {
        auto* it = new QListWidgetItem(QStringLiteral("%1  %2").arg(e.ref, e.subject), m_list);
        it->setData(Qt::UserRole, e.ref);
    }
}

QString GitStashDialog::selectedRef() const
{
    auto* it = m_list->currentItem();
    return it ? it->data(Qt::UserRole).toString() : QString();
}

void GitStashDialog::onPush()
{
    QString err;
    if (!GitOps::stashPush(m_anchor, m_message->text(), m_untracked->isChecked(), &err)) {
        QMessageBox::warning(this, tr("Stash"), err);
        return;
    }
    m_message->clear();
    rebuild();
}

void GitStashDialog::onApply()
{
    const QString ref = selectedRef();
    if (ref.isEmpty()) return;
    QString err;
    if (!GitOps::stashApply(m_anchor, ref, /*drop*/ false, &err)) {
        QMessageBox::warning(this, tr("Stash"), err);
    }
    rebuild();
}

void GitStashDialog::onPop()
{
    const QString ref = selectedRef();
    if (ref.isEmpty()) return;
    QString err;
    if (!GitOps::stashApply(m_anchor, ref, /*drop*/ true, &err)) {
        QMessageBox::warning(this, tr("Stash"), err);
    }
    rebuild();
}

void GitStashDialog::onDrop()
{
    const QString ref = selectedRef();
    if (ref.isEmpty()) return;
    if (QMessageBox::question(this, tr("Stash"),
            tr("Apagar %1?").arg(ref)) != QMessageBox::Yes) return;
    QString err;
    if (!GitOps::stashDrop(m_anchor, ref, &err)) {
        QMessageBox::warning(this, tr("Stash"), err);
    }
    rebuild();
}
