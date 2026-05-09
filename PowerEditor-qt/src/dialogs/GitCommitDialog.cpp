#include "GitCommitDialog.h"

#include "../GitOps.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

GitCommitDialog::GitCommitDialog(const QString& filePathInRepo, QWidget* parent)
    : QDialog(parent), m_anchorFile(filePathInRepo)
{
    setWindowTitle(tr("Commit"));
    resize(700, 520);

    m_status = new QLabel(this);

    m_files = new QTreeWidget(this);
    m_files->setHeaderLabels({ tr("Stg"), tr("Mod"), tr("Arquivo") });
    m_files->setRootIsDecorated(false);
    m_files->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_files->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_files->header()->setSectionResizeMode(2, QHeaderView::Stretch);

    m_message = new QPlainTextEdit(this);
    m_message->setPlaceholderText(tr("Mensagem do commit (Ctrl+Enter para commitar)"));

    m_refreshBtn = new QPushButton(tr("Atualizar"), this);
    m_commitBtn  = new QPushButton(tr("Commit"), this);

    auto* row = new QHBoxLayout();
    row->addWidget(m_refreshBtn);
    row->addStretch(1);
    row->addWidget(m_commitBtn);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* lay = new QVBoxLayout(this);
    lay->addWidget(m_status);
    lay->addWidget(m_files, /*stretch*/2);
    lay->addWidget(new QLabel(tr("Mensagem:"), this));
    lay->addWidget(m_message, /*stretch*/1);
    lay->addLayout(row);
    lay->addWidget(bb);

    connect(m_files,      &QTreeWidget::itemChanged, this, &GitCommitDialog::onItemChanged);
    connect(m_refreshBtn, &QPushButton::clicked,     this, &GitCommitDialog::onRefresh);
    connect(m_commitBtn,  &QPushButton::clicked,     this, &GitCommitDialog::onCommitClicked);

    rebuildList();
}

bool GitCommitDialog::isStaged(const QString& staged, const QString& /*unstaged*/) const
{
    return !staged.isEmpty() && staged != QStringLiteral(" ") && staged != QStringLiteral("?");
}

void GitCommitDialog::rebuildList()
{
    m_suppressItemSignals = true;
    m_files->clear();

    QList<GitOps::StatusRow> rows;
    QString err;
    if (!GitOps::status(m_anchorFile, rows, &err)) {
        m_status->setText(tr("Erro: %1").arg(err));
        m_suppressItemSignals = false;
        return;
    }
    if (rows.isEmpty()) {
        m_status->setText(tr("Nada para commitar — árvore limpa."));
    } else {
        m_status->setText(tr("%1 arquivo(s) com modificações.").arg(rows.size()));
    }

    for (const GitOps::StatusRow& r : rows) {
        auto* it = new QTreeWidgetItem(m_files);
        it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
        it->setCheckState(0, isStaged(r.staged, r.unstaged) ? Qt::Checked : Qt::Unchecked);
        it->setText(1, r.unstaged.trimmed().isEmpty() ? r.staged : r.unstaged);
        it->setText(2, r.path);
        it->setData(2, Qt::UserRole, r.path);
    }
    m_suppressItemSignals = false;
}

void GitCommitDialog::onItemChanged(QTreeWidgetItem* item, int col)
{
    if (m_suppressItemSignals) return;
    if (col != 0 || !item) return;
    const QString path = item->data(2, Qt::UserRole).toString();
    if (path.isEmpty()) return;
    QString err;
    bool ok = false;
    if (item->checkState(0) == Qt::Checked) {
        ok = GitOps::add(m_anchorFile, path, &err);
    } else {
        ok = GitOps::resetIndex(m_anchorFile, path, &err);
    }
    if (!ok) {
        QMessageBox::warning(this, tr("Git"), err);
        rebuildList();   // re-sync UI with reality
    }
}

void GitCommitDialog::onCommitClicked()
{
    const QString msg = m_message->toPlainText().trimmed();
    if (msg.isEmpty()) {
        QMessageBox::warning(this, tr("Commit"), tr("Mensagem vazia."));
        return;
    }
    QString err;
    if (!GitOps::commit(m_anchorFile, msg, &err)) {
        QMessageBox::warning(this, tr("Commit"), err);
        return;
    }
    QMessageBox::information(this, tr("Commit"), tr("Commit criado com sucesso."));
    accept();
}

void GitCommitDialog::onRefresh() { rebuildList(); }
