#include "NotesPanel.h"

#include <QAction>
#include <QColorDialog>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QTextCharFormat>
#include <QTextEdit>
#include <QTextListFormat>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

namespace {
// QTreeWidgetItem custom data roles. The HTML body is stored on the item
// itself (UserRole+0) so we don't have to touch the disk on every selection.
constexpr int RoleHtml = Qt::UserRole + 0;
}

NotesPanel::NotesPanel(QWidget* parent) : QDockWidget(tr("Notas"), parent)
{
    setObjectName(QStringLiteral("NotesPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    m_split = new QSplitter(Qt::Horizontal, root);

    // ---- Left: tree of notes ----
    auto* leftWrap = new QWidget(m_split);
    auto* leftLay  = new QVBoxLayout(leftWrap);
    leftLay->setContentsMargins(0, 0, 0, 0);

    auto* treeBar = new QToolBar(leftWrap);
    treeBar->addAction(tr("Filho"),  this, &NotesPanel::onNewChild);
    treeBar->addAction(tr("Irmão"),  this, &NotesPanel::onNewSibling);
    treeBar->addAction(tr("Renomear"), this, &NotesPanel::onRename);
    treeBar->addAction(tr("Apagar"), this, &NotesPanel::onDelete);
    treeBar->addSeparator();
    treeBar->addAction(QStringLiteral("↑"), this, &NotesPanel::onMoveUp);
    treeBar->addAction(QStringLiteral("↓"), this, &NotesPanel::onMoveDown);

    m_tree = new QTreeWidget(leftWrap);
    m_tree->setHeaderHidden(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);

    leftLay->addWidget(treeBar);
    leftLay->addWidget(m_tree, 1);

    // ---- Right: rich-text editor ----
    auto* rightWrap = new QWidget(m_split);
    auto* rightLay  = new QVBoxLayout(rightWrap);
    rightLay->setContentsMargins(0, 0, 0, 0);

    m_titleEdit = new QLineEdit(rightWrap);
    m_titleEdit->setPlaceholderText(tr("Título"));

    m_fmtBar = new QToolBar(rightWrap);
    auto addFmt = [&](const QString& label, auto slot, const QString& tip = {}) {
        auto* a = m_fmtBar->addAction(label, this, slot);
        if (!tip.isEmpty()) a->setToolTip(tip);
        return a;
    };
    addFmt(QStringLiteral("B"),  &NotesPanel::onBold,      tr("Negrito (Ctrl+B)"));
    addFmt(QStringLiteral("I"),  &NotesPanel::onItalic,    tr("Itálico (Ctrl+I)"));
    addFmt(QStringLiteral("U"),  &NotesPanel::onUnderline, tr("Sublinhado"));
    addFmt(QStringLiteral("S"),  &NotesPanel::onStrike,    tr("Tachado"));
    m_fmtBar->addSeparator();
    {
        auto* h1 = m_fmtBar->addAction(QStringLiteral("H1"), this, &NotesPanel::onHeading);
        h1->setData(1);
        auto* h2 = m_fmtBar->addAction(QStringLiteral("H2"), this, &NotesPanel::onHeading);
        h2->setData(2);
        auto* h3 = m_fmtBar->addAction(QStringLiteral("H3"), this, &NotesPanel::onHeading);
        h3->setData(3);
    }
    m_fmtBar->addSeparator();
    addFmt(QStringLiteral("• Lista"), &NotesPanel::onBulletList);
    addFmt(QStringLiteral("1. Lista"), &NotesPanel::onNumberedList);
    m_fmtBar->addSeparator();
    addFmt(tr("Cor…"),   &NotesPanel::onForeColor);
    addFmt(tr("Link…"),  &NotesPanel::onInsertLink);
    addFmt(QStringLiteral("</>"),   &NotesPanel::onCodeBlock);

    m_body = new QTextEdit(rightWrap);
    m_body->setAcceptRichText(true);
    m_body->setPlaceholderText(tr("Escreva aqui — Markdown / rich text. Ctrl+B/I para formatar."));
    QFont f = m_body->font();
    f.setPointSize(11);
    m_body->setFont(f);

    m_status = new QLabel(rightWrap);

    rightLay->addWidget(m_titleEdit);
    rightLay->addWidget(m_fmtBar);
    rightLay->addWidget(m_body, 1);
    rightLay->addWidget(m_status);

    m_split->addWidget(leftWrap);
    m_split->addWidget(rightWrap);
    m_split->setStretchFactor(0, 1);
    m_split->setStretchFactor(1, 3);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(2, 2, 2, 2);
    lay->addWidget(m_split);
    setWidget(root);

    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(500);
    connect(m_saveTimer, &QTimer::timeout, this, &NotesPanel::onFlushPending);

    connect(m_tree, &QTreeWidget::currentItemChanged,
            this, [this](QTreeWidgetItem*, QTreeWidgetItem*) { onTreeSelectionChanged(); });
    connect(m_body, &QTextEdit::textChanged, this, &NotesPanel::onBodyChanged);
    connect(m_titleEdit, &QLineEdit::editingFinished, this, [this]() {
        if (auto* it = currentNode()) {
            it->setText(0, m_titleEdit->text());
            saveToDisk();
        }
    });

    // Ctrl+B / Ctrl+I shortcuts as expected from a rich text editor.
    auto* aBold   = new QAction(this); aBold->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_B));
    connect(aBold,   &QAction::triggered, this, &NotesPanel::onBold);   m_body->addAction(aBold);
    auto* aItalic = new QAction(this); aItalic->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_I));
    connect(aItalic, &QAction::triggered, this, &NotesPanel::onItalic); m_body->addAction(aItalic);

    loadFromDisk();
    if (m_tree->topLevelItemCount() == 0) {
        auto* welcome = makeNode(nullptr, tr("Notas"),
            QStringLiteral("<h2>Bem-vindo</h2><p>Use o painel para tomar notas hierárquicas. "
                           "Botão <b>Filho</b> cria sub-nó; <b>Irmão</b> cria nó no mesmo nível.</p>"));
        m_tree->setCurrentItem(welcome);
    } else {
        m_tree->setCurrentItem(m_tree->topLevelItem(0));
    }
}

NotesPanel::~NotesPanel()
{
    persistCurrentBody();
    saveToDisk();
}

QString NotesPanel::notesFilePath() const
{
    QDir d(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation));
    if (!d.exists()) d.mkpath(".");
    return d.absoluteFilePath(QStringLiteral("cherry-notes.json"));
}

QTreeWidgetItem* NotesPanel::makeNode(QTreeWidgetItem* parent, const QString& title,
                                      const QString& html)
{
    auto* it = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(m_tree);
    it->setText(0, title);
    it->setData(0, RoleHtml, html);
    it->setExpanded(true);
    return it;
}

QTreeWidgetItem* NotesPanel::currentNode() const
{
    return m_tree->currentItem();
}

void NotesPanel::onTreeSelectionChanged()
{
    if (m_suppressTreeSignal) return;
    persistCurrentBody();   // flush previously edited node
    m_suppressBodySignal = true;
    auto* it = currentNode();
    if (!it) {
        m_titleEdit->clear();
        m_body->clear();
    } else {
        m_titleEdit->setText(it->text(0));
        m_body->setHtml(it->data(0, RoleHtml).toString());
    }
    m_suppressBodySignal = false;
}

void NotesPanel::onBodyChanged()
{
    if (m_suppressBodySignal) return;
    m_saveTimer->start();
}

void NotesPanel::persistCurrentBody()
{
    auto* it = currentNode();
    if (!it) return;
    it->setData(0, RoleHtml, m_body->toHtml());
}

void NotesPanel::onFlushPending()
{
    persistCurrentBody();
    saveToDisk();
    m_status->setText(tr("Salvo: %1").arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
}

// ---- tree ops ----

void NotesPanel::onNewChild()
{
    auto* it = currentNode();
    auto* node = makeNode(it, tr("Nova nota"), QString());
    m_tree->setCurrentItem(node);
    saveToDisk();
}
void NotesPanel::onNewSibling()
{
    auto* it = currentNode();
    auto* node = makeNode(it ? it->parent() : nullptr, tr("Nova nota"), QString());
    m_tree->setCurrentItem(node);
    saveToDisk();
}
void NotesPanel::onRename()
{
    auto* it = currentNode();
    if (!it) return;
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Renomear"),
        tr("Novo título:"), QLineEdit::Normal, it->text(0), &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    it->setText(0, name.trimmed());
    m_titleEdit->setText(name.trimmed());
    saveToDisk();
}
void NotesPanel::onDelete()
{
    auto* it = currentNode();
    if (!it) return;
    if (QMessageBox::question(this, tr("Apagar nota"),
            tr("Apagar \"%1\" e seus descendentes?").arg(it->text(0))) != QMessageBox::Yes) return;
    delete it;
    saveToDisk();
}
void NotesPanel::onMoveUp()
{
    auto* it = currentNode(); if (!it) return;
    auto* p = it->parent();
    if (p) {
        int i = p->indexOfChild(it);
        if (i > 0) { p->takeChild(i); p->insertChild(i - 1, it); m_tree->setCurrentItem(it); }
    } else {
        int i = m_tree->indexOfTopLevelItem(it);
        if (i > 0) { m_tree->takeTopLevelItem(i); m_tree->insertTopLevelItem(i - 1, it); m_tree->setCurrentItem(it); }
    }
    saveToDisk();
}
void NotesPanel::onMoveDown()
{
    auto* it = currentNode(); if (!it) return;
    auto* p = it->parent();
    if (p) {
        int i = p->indexOfChild(it);
        if (i + 1 < p->childCount()) { p->takeChild(i); p->insertChild(i + 1, it); m_tree->setCurrentItem(it); }
    } else {
        int i = m_tree->indexOfTopLevelItem(it);
        if (i + 1 < m_tree->topLevelItemCount()) {
            m_tree->takeTopLevelItem(i); m_tree->insertTopLevelItem(i + 1, it); m_tree->setCurrentItem(it);
        }
    }
    saveToDisk();
}

// ---- formatting ops ----

void NotesPanel::onBold()
{
    QTextCharFormat f;
    f.setFontWeight(m_body->fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
    m_body->mergeCurrentCharFormat(f);
}
void NotesPanel::onItalic()
{
    QTextCharFormat f; f.setFontItalic(!m_body->fontItalic());
    m_body->mergeCurrentCharFormat(f);
}
void NotesPanel::onUnderline()
{
    QTextCharFormat f; f.setFontUnderline(!m_body->fontUnderline());
    m_body->mergeCurrentCharFormat(f);
}
void NotesPanel::onStrike()
{
    QTextCharFormat f;
    f.setFontStrikeOut(!m_body->currentCharFormat().fontStrikeOut());
    m_body->mergeCurrentCharFormat(f);
}
void NotesPanel::onHeading()
{
    auto* a = qobject_cast<QAction*>(sender());
    if (!a) return;
    const int level = a->data().toInt();
    QTextCharFormat f;
    f.setFontWeight(QFont::Bold);
    // Heading point size scales: H1=18, H2=15, H3=13.
    const int pt = (level == 1) ? 18 : (level == 2) ? 15 : 13;
    f.setFontPointSize(pt);
    m_body->mergeCurrentCharFormat(f);
}
void NotesPanel::onBulletList()
{
    QTextCursor c = m_body->textCursor();
    c.beginEditBlock();
    QTextListFormat lf; lf.setStyle(QTextListFormat::ListDisc);
    c.createList(lf);
    c.endEditBlock();
}
void NotesPanel::onNumberedList()
{
    QTextCursor c = m_body->textCursor();
    c.beginEditBlock();
    QTextListFormat lf; lf.setStyle(QTextListFormat::ListDecimal);
    c.createList(lf);
    c.endEditBlock();
}
void NotesPanel::onForeColor()
{
    QColor c = QColorDialog::getColor(Qt::black, this, tr("Cor do texto"));
    if (!c.isValid()) return;
    QTextCharFormat f; f.setForeground(c);
    m_body->mergeCurrentCharFormat(f);
}
void NotesPanel::onInsertLink()
{
    bool ok = false;
    const QString href = QInputDialog::getText(this, tr("Inserir link"),
        tr("URL:"), QLineEdit::Normal, QStringLiteral("https://"), &ok);
    if (!ok || href.isEmpty()) return;
    QTextCharFormat f;
    f.setAnchor(true);
    f.setAnchorHref(href);
    f.setForeground(QColor("#3b82f6"));
    f.setFontUnderline(true);
    QTextCursor c = m_body->textCursor();
    if (c.hasSelection()) c.mergeCharFormat(f);
    else                  c.insertText(href, f);
}
void NotesPanel::onCodeBlock()
{
    // Wrap selection (or insert empty) as monospace + light background.
    QTextCharFormat f;
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    f.setFont(mono);
    f.setBackground(QColor("#1e2a36"));
    f.setForeground(QColor("#dcdcdc"));
    QTextCursor c = m_body->textCursor();
    if (c.hasSelection()) c.mergeCharFormat(f);
    else                  c.insertText(QStringLiteral("código"), f);
}

// ---- persistence ----

QJsonObject* NotesPanel::serialize(QTreeWidgetItem* it) const
{
    if (!it) return nullptr;
    auto* o = new QJsonObject();
    o->insert(QStringLiteral("title"), it->text(0));
    o->insert(QStringLiteral("html"),  it->data(0, RoleHtml).toString());
    QJsonArray children;
    for (int i = 0; i < it->childCount(); ++i) {
        QJsonObject* c = serialize(it->child(i));
        if (c) { children.append(*c); delete c; }
    }
    o->insert(QStringLiteral("children"), children);
    return o;
}

void NotesPanel::deserialize(const QJsonObject& obj, QTreeWidgetItem* parent)
{
    auto* it = makeNode(parent, obj.value(QStringLiteral("title")).toString(),
                                obj.value(QStringLiteral("html")).toString());
    for (const QJsonValue& v : obj.value(QStringLiteral("children")).toArray()) {
        if (v.isObject()) deserialize(v.toObject(), it);
    }
}

void NotesPanel::saveToDisk() const
{
    QJsonArray roots;
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        QJsonObject* o = serialize(m_tree->topLevelItem(i));
        if (o) { roots.append(*o); delete o; }
    }
    QJsonObject root;
    root.insert(QStringLiteral("nodes"), roots);
    QFile f(notesFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
}

void NotesPanel::loadFromDisk()
{
    QFile f(notesFilePath());
    if (!f.exists() || !f.open(QIODevice::ReadOnly)) return;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return;
    const QJsonArray nodes = doc.object().value(QStringLiteral("nodes")).toArray();
    for (const QJsonValue& v : nodes) {
        if (v.isObject()) deserialize(v.toObject(), nullptr);
    }
}
