#include "GitBranchDialog.h"

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

GitBranchDialog::GitBranchDialog(const QString& anchorFilePath, QWidget* parent)
    : QDialog(parent), m_anchor(anchorFilePath)
{
    setWindowTitle(tr("Branches"));
    resize(520, 460);

    m_status = new QLabel(this);
    m_status->setText(tr("Branch atual: %1").arg(GitOps::currentBranch(anchorFilePath)));

    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);

    m_remoteChk   = new QCheckBox(tr("Incluir remotos"), this);
    m_checkoutBtn = new QPushButton(tr("Checkout"), this);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(m_remoteChk);
    row1->addStretch(1);
    row1->addWidget(m_checkoutBtn);

    m_newName  = new QLineEdit(this);
    m_newName->setPlaceholderText(tr("Nome da nova branch (do branch selecionado ou HEAD)"));
    m_createBtn = new QPushButton(tr("Criar e mudar"), this);

    auto* row2 = new QHBoxLayout();
    row2->addWidget(m_newName, 1);
    row2->addWidget(m_createBtn);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::accept);

    auto* lay = new QVBoxLayout(this);
    lay->addWidget(m_status);
    lay->addWidget(m_list, 1);
    lay->addLayout(row1);
    lay->addWidget(new QLabel(tr("Criar nova branch:"), this));
    lay->addLayout(row2);
    lay->addWidget(bb);

    connect(m_remoteChk,   &QCheckBox::toggled,         this, &GitBranchDialog::onIncludeRemoteToggled);
    connect(m_checkoutBtn, &QPushButton::clicked,       this, &GitBranchDialog::onCheckout);
    connect(m_createBtn,   &QPushButton::clicked,       this, &GitBranchDialog::onCreate);
    connect(m_list,        &QListWidget::itemDoubleClicked, this, &GitBranchDialog::onItemDoubleClicked);

    rebuild();
}

void GitBranchDialog::onIncludeRemoteToggled() { rebuild(); }

void GitBranchDialog::rebuild()
{
    m_list->clear();
    QList<GitOps::Branch> branches;
    QString err;
    const bool remote = m_remoteChk && m_remoteChk->isChecked();
    if (!GitOps::branches(m_anchor, remote, branches, &err)) {
        m_status->setText(tr("Erro: %1").arg(err));
        return;
    }
    for (const GitOps::Branch& b : branches) {
        QString label = b.name;
        if (b.isCurrent) label.prepend(QStringLiteral("● "));
        else             label.prepend(QStringLiteral("  "));
        if (b.isRemote && !label.contains(QStringLiteral("[remoto]")))
            label += QStringLiteral("   [remoto]");
        auto* it = new QListWidgetItem(label, m_list);
        it->setData(Qt::UserRole, b.name);
        if (b.isCurrent) {
            QFont f = it->font(); f.setBold(true); it->setFont(f);
        }
    }
}

void GitBranchDialog::onCheckout()
{
    auto* it = m_list->currentItem();
    if (!it) return;
    const QString name = it->data(Qt::UserRole).toString();
    QString err;
    if (!GitOps::checkoutBranch(m_anchor, name, &err)) {
        QMessageBox::warning(this, tr("Branch"), err);
        return;
    }
    m_status->setText(tr("Branch atual: %1").arg(GitOps::currentBranch(m_anchor)));
    rebuild();
}

void GitBranchDialog::onItemDoubleClicked(QListWidgetItem*) { onCheckout(); }

void GitBranchDialog::onCreate()
{
    const QString name = m_newName->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Branch"), tr("Digite o nome da nova branch."));
        return;
    }
    QString from;
    if (auto* it = m_list->currentItem()) from = it->data(Qt::UserRole).toString();
    QString err;
    if (!GitOps::createBranch(m_anchor, name, from, &err)) {
        QMessageBox::warning(this, tr("Branch"), err);
        return;
    }
    m_newName->clear();
    m_status->setText(tr("Branch atual: %1").arg(GitOps::currentBranch(m_anchor)));
    rebuild();
}
