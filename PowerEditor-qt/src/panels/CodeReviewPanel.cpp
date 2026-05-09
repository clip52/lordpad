#include "CodeReviewPanel.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

CodeReviewPanel::CodeReviewPanel(QWidget* parent) : QDockWidget(tr("Review"), parent)
{
    setObjectName(QStringLiteral("CodeReviewPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    m_list = new QTreeWidget(root);
    m_list->setHeaderLabels({ tr("Linha"), tr("Comentário") });
    m_list->setRootIsDecorated(false);
    m_list->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_list->header()->setSectionResizeMode(1, QHeaderView::Stretch);

    m_text = new QPlainTextEdit(root);
    m_text->setReadOnly(true);

    m_addBtn = new QPushButton(tr("Adicionar"), root);
    m_delBtn = new QPushButton(tr("Apagar"), root);
    m_status = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(m_addBtn); row->addWidget(m_delBtn); row->addStretch(1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addWidget(m_list, 2);
    lay->addWidget(m_text, 1);
    lay->addLayout(row);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_addBtn, &QPushButton::clicked, this, &CodeReviewPanel::onAdd);
    connect(m_delBtn, &QPushButton::clicked, this, &CodeReviewPanel::onDelete);
    connect(m_list, &QTreeWidget::itemActivated, this, &CodeReviewPanel::onItemActivated);
    connect(m_list, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* cur, QTreeWidgetItem*){
        m_text->setPlainText(cur ? cur->data(1, Qt::UserRole).toString() : QString());
    });
}

void CodeReviewPanel::setActiveFile(const QString& path)
{
    if (m_path == path) return;
    m_path = path;
    loadForActive();
    m_status->setText(path.isEmpty() ? tr("(sem arquivo)") : path);
}

void CodeReviewPanel::loadForActive()
{
    m_list->clear();
    if (m_path.isEmpty()) return;
    QSettings s; s.beginGroup(QStringLiteral("CodeReview"));
    const QString raw = s.value(m_path).toString();
    s.endGroup();
    if (raw.isEmpty()) return;
    const QJsonDocument doc = QJsonDocument::fromJson(raw.toUtf8());
    for (const QJsonValue& v : doc.array()) {
        const QJsonObject o = v.toObject();
        auto* it = new QTreeWidgetItem(m_list);
        it->setText(0, QString::number(o.value(QStringLiteral("line")).toInt()));
        const QString text = o.value(QStringLiteral("text")).toString();
        QString brief = text.section('\n', 0, 0);
        if (brief.size() > 80) brief = brief.left(80) + QStringLiteral("…");
        it->setText(1, brief);
        it->setData(0, Qt::UserRole, o.value(QStringLiteral("line")).toInt());
        it->setData(1, Qt::UserRole, text);
    }
}

void CodeReviewPanel::persistForActive()
{
    if (m_path.isEmpty()) return;
    QJsonArray arr;
    for (int i = 0; i < m_list->topLevelItemCount(); ++i) {
        auto* it = m_list->topLevelItem(i);
        QJsonObject o;
        o.insert(QStringLiteral("line"), it->data(0, Qt::UserRole).toInt());
        o.insert(QStringLiteral("text"), it->data(1, Qt::UserRole).toString());
        arr.append(o);
    }
    QSettings s; s.beginGroup(QStringLiteral("CodeReview"));
    s.setValue(m_path, QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
    s.endGroup();
}

void CodeReviewPanel::onAdd()
{
    if (m_path.isEmpty()) { m_status->setText(tr("Sem arquivo ativo.")); return; }
    bool ok = false;
    const int line = QInputDialog::getInt(this, tr("Linha"), tr("Linha:"), 1, 1, 1'000'000, 1, &ok);
    if (!ok) return;
    const QString text = QInputDialog::getMultiLineText(this, tr("Comentário"),
        tr("Texto:"), QString(), &ok);
    if (!ok || text.isEmpty()) return;
    auto* it = new QTreeWidgetItem(m_list);
    it->setText(0, QString::number(line));
    QString brief = text.section('\n', 0, 0);
    if (brief.size() > 80) brief = brief.left(80) + QStringLiteral("…");
    it->setText(1, brief);
    it->setData(0, Qt::UserRole, line);
    it->setData(1, Qt::UserRole, text);
    persistForActive();
}

void CodeReviewPanel::onDelete()
{
    auto* it = m_list->currentItem();
    if (!it) return;
    delete m_list->takeTopLevelItem(m_list->indexOfTopLevelItem(it));
    persistForActive();
}

void CodeReviewPanel::onItemActivated(QTreeWidgetItem* it, int)
{
    if (!it) return;
    emit gotoLineRequested(it->data(0, Qt::UserRole).toInt());
}
