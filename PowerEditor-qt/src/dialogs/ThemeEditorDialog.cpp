#include "ThemeEditorDialog.h"

#include <QColorDialog>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <ScintillaEdit.h>

namespace {

struct Category {
    QString  key;          // settings/programmatic key
    QString  display;      // pt-BR label for the table
    QRgb     defaultFore;
    QRgb     defaultBack;
};
const QList<Category> kCategories = {
    { QStringLiteral("default"),    QStringLiteral("Default"),     0xD8D8D8, 0x1E1E1E },
    { QStringLiteral("comment"),    QStringLiteral("Comentário"),  0x808080, 0x1E1E1E },
    { QStringLiteral("string"),     QStringLiteral("String"),      0xCE9178, 0x1E1E1E },
    { QStringLiteral("number"),     QStringLiteral("Número"),      0xB5CEA8, 0x1E1E1E },
    { QStringLiteral("keyword"),    QStringLiteral("Keyword"),     0xC586C0, 0x1E1E1E },
    { QStringLiteral("identifier"), QStringLiteral("Identificador"),0xD8D8D8, 0x1E1E1E },
    { QStringLiteral("operator"),   QStringLiteral("Operador"),    0xD4D4D4, 0x1E1E1E },
    { QStringLiteral("preproc"),    QStringLiteral("Preprocessor"),0x569CD6, 0x1E1E1E },
    { QStringLiteral("linenumber"), QStringLiteral("Nº de linha"), 0x858585, 0x252526 },
};

// Convert a 0xRRGGBB QRgb to Scintilla's 0x00BBGGRR int.
sptr_t toSciColor(QRgb rgb) {
    const int r = (rgb >> 16) & 0xFF;
    const int g = (rgb >>  8) & 0xFF;
    const int b = (rgb >>  0) & 0xFF;
    return (b << 16) | (g << 8) | r;
}
QRgb fromSciColor(sptr_t c) {
    const int b = (c >> 16) & 0xFF;
    const int g = (c >>  8) & 0xFF;
    const int r = (c >>  0) & 0xFF;
    return qRgb(r, g, b);
}

} // namespace

ThemeEditorDialog::ThemeEditorDialog(ScintillaEdit* editor, const QString& lexerName,
                                     QWidget* parent)
    : QDialog(parent), m_editor(editor), m_lexerName(lexerName)
{
    setWindowTitle(tr("Editor de tema (%1)").arg(lexerName.isEmpty() ? tr("genérico") : lexerName));
    resize(600, 460);

    m_tree = new QTreeWidget(this);
    m_tree->setColumnCount(3);
    m_tree->setHeaderLabels({ tr("Categoria"), tr("Texto"), tr("Fundo") });
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_tree->setRootIsDecorated(false);

    m_pickFore = new QPushButton(tr("Cor do texto…"), this);
    m_pickBack = new QPushButton(tr("Cor do fundo…"), this);
    m_applyAll = new QPushButton(tr("Reaplica todas"), this);
    m_reset    = new QPushButton(tr("Resetar categoria"), this);

    m_status = new QLabel(this);

    auto* row = new QHBoxLayout();
    row->addWidget(m_pickFore);
    row->addWidget(m_pickBack);
    row->addWidget(m_reset);
    row->addStretch(1);
    row->addWidget(m_applyAll);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::accept);

    auto* lay = new QVBoxLayout(this);
    lay->addWidget(new QLabel(tr("Selecione uma categoria, depois ajuste as cores. As mudanças são aplicadas no editor ativo."), this));
    lay->addWidget(m_tree, 1);
    lay->addLayout(row);
    lay->addWidget(m_status);
    lay->addWidget(bb);

    connect(m_pickFore, &QPushButton::clicked, this, &ThemeEditorDialog::onPickFore);
    connect(m_pickBack, &QPushButton::clicked, this, &ThemeEditorDialog::onPickBack);
    connect(m_applyAll, &QPushButton::clicked, this, &ThemeEditorDialog::onApplyAll);
    connect(m_reset,    &QPushButton::clicked, this, &ThemeEditorDialog::onResetCategory);

    rebuild();
}

void ThemeEditorDialog::rebuild()
{
    m_tree->clear();
    for (const Category& c : kCategories) {
        int fore = c.defaultFore, back = c.defaultBack;
        loadPersisted(c.key, fore, back);

        auto* it = new QTreeWidgetItem(m_tree);
        it->setText(0, c.display);
        it->setData(0, Qt::UserRole, c.key);
        it->setBackground(1, QBrush(QColor::fromRgb(fore)));
        it->setBackground(2, QBrush(QColor::fromRgb(back)));
        it->setText(1, QString::asprintf("#%06X", fore & 0xFFFFFF));
        it->setText(2, QString::asprintf("#%06X", back & 0xFFFFFF));

        // Apply on open so the user sees the persisted theme immediately.
        applyColors(c.key, fore, back);
    }
    if (m_tree->topLevelItemCount() > 0) m_tree->setCurrentItem(m_tree->topLevelItem(0));
}

QList<int> ThemeEditorDialog::stylesForCategory(const QString& categoryKey) const
{
    // Map each logical category to the relevant style numbers per lexer.
    // Numbers come from the SCE_*_* enums in Lexilla; we hand-pick the
    // important ones rather than try to enumerate every constant.
    if (categoryKey == QStringLiteral("default"))    return { 32 /*STYLE_DEFAULT*/, 0 };
    if (categoryKey == QStringLiteral("linenumber")) return { 33 /*STYLE_LINENUMBER*/ };

    if (m_lexerName == QStringLiteral("cpp") || m_lexerName == QStringLiteral("c")) {
        if (categoryKey == QStringLiteral("comment"))    return { 1, 2, 3, 15 };       // SCE_C_COMMENT*
        if (categoryKey == QStringLiteral("string"))     return { 6, 7, 12, 13 };
        if (categoryKey == QStringLiteral("number"))     return { 4 };
        if (categoryKey == QStringLiteral("keyword"))    return { 5, 16 };
        if (categoryKey == QStringLiteral("identifier")) return { 11 };
        if (categoryKey == QStringLiteral("operator"))   return { 10 };
        if (categoryKey == QStringLiteral("preproc"))    return { 9 };
    } else if (m_lexerName == QStringLiteral("python")) {
        if (categoryKey == QStringLiteral("comment"))    return { 1, 12 };
        if (categoryKey == QStringLiteral("string"))     return { 3, 4, 6, 7, 13 };
        if (categoryKey == QStringLiteral("number"))     return { 2 };
        if (categoryKey == QStringLiteral("keyword"))    return { 5 };
        if (categoryKey == QStringLiteral("identifier")) return { 11 };
        if (categoryKey == QStringLiteral("operator"))   return { 10 };
    } else if (m_lexerName == QStringLiteral("javascript")
            || m_lexerName == QStringLiteral("typescript")) {
        // CPP lexer reused for JS/TS; same constants.
        if (categoryKey == QStringLiteral("comment"))    return { 1, 2, 3, 15 };
        if (categoryKey == QStringLiteral("string"))     return { 6, 7, 12, 13 };
        if (categoryKey == QStringLiteral("number"))     return { 4 };
        if (categoryKey == QStringLiteral("keyword"))    return { 5 };
        if (categoryKey == QStringLiteral("identifier")) return { 11 };
        if (categoryKey == QStringLiteral("operator"))   return { 10 };
    }
    return {};
}

void ThemeEditorDialog::applyColors(const QString& categoryKey, int fore, int back)
{
    if (!m_editor) return;
    const QList<int> styles = stylesForCategory(categoryKey);
    for (int sty : styles) {
        m_editor->styleSetFore(sty, toSciColor(fore));
        m_editor->styleSetBack(sty, toSciColor(back));
    }
    m_editor->colourise(0, -1);   // force repaint
}

void ThemeEditorDialog::onPickFore()
{
    auto* it = m_tree->currentItem();
    if (!it) return;
    const QString key = it->data(0, Qt::UserRole).toString();
    QColor cur = it->background(1).color();
    QColor c = QColorDialog::getColor(cur, this, tr("Cor do texto"));
    if (!c.isValid()) return;
    const int fore = c.rgb() & 0xFFFFFF;
    const int back = it->background(2).color().rgb() & 0xFFFFFF;
    it->setBackground(1, QBrush(c));
    it->setText(1, QString::asprintf("#%06X", fore));
    applyColors(key, fore, back);
    persistCategory(key);
    m_status->setText(tr("Aplicado: texto da categoria %1.").arg(it->text(0)));
}

void ThemeEditorDialog::onPickBack()
{
    auto* it = m_tree->currentItem();
    if (!it) return;
    const QString key = it->data(0, Qt::UserRole).toString();
    QColor cur = it->background(2).color();
    QColor c = QColorDialog::getColor(cur, this, tr("Cor do fundo"));
    if (!c.isValid()) return;
    const int fore = it->background(1).color().rgb() & 0xFFFFFF;
    const int back = c.rgb() & 0xFFFFFF;
    it->setBackground(2, QBrush(c));
    it->setText(2, QString::asprintf("#%06X", back));
    applyColors(key, fore, back);
    persistCategory(key);
    m_status->setText(tr("Aplicado: fundo da categoria %1.").arg(it->text(0)));
}

void ThemeEditorDialog::onApplyAll()
{
    for (int i = 0; i < m_tree->topLevelItemCount(); ++i) {
        auto* it = m_tree->topLevelItem(i);
        const QString key = it->data(0, Qt::UserRole).toString();
        const int fore = it->background(1).color().rgb() & 0xFFFFFF;
        const int back = it->background(2).color().rgb() & 0xFFFFFF;
        applyColors(key, fore, back);
    }
    m_status->setText(tr("Todas as categorias reaplicadas."));
}

void ThemeEditorDialog::onResetCategory()
{
    auto* it = m_tree->currentItem();
    if (!it) return;
    const QString key = it->data(0, Qt::UserRole).toString();
    for (const Category& c : kCategories) {
        if (c.key != key) continue;
        QSettings s;
        s.beginGroup(QStringLiteral("ThemeEditor/") + m_lexerName + QStringLiteral("/") + key);
        s.remove(QString());
        s.endGroup();
        it->setBackground(1, QBrush(QColor::fromRgb(c.defaultFore)));
        it->setBackground(2, QBrush(QColor::fromRgb(c.defaultBack)));
        it->setText(1, QString::asprintf("#%06X", c.defaultFore));
        it->setText(2, QString::asprintf("#%06X", c.defaultBack));
        applyColors(key, c.defaultFore, c.defaultBack);
        m_status->setText(tr("Categoria %1 resetada.").arg(it->text(0)));
        return;
    }
}

void ThemeEditorDialog::persistCategory(const QString& categoryKey)
{
    auto* it = m_tree->currentItem();
    if (!it) return;
    const int fore = it->background(1).color().rgb() & 0xFFFFFF;
    const int back = it->background(2).color().rgb() & 0xFFFFFF;
    QSettings s;
    s.beginGroup(QStringLiteral("ThemeEditor/") + m_lexerName + QStringLiteral("/") + categoryKey);
    s.setValue(QStringLiteral("fore"), fore);
    s.setValue(QStringLiteral("back"), back);
    s.endGroup();
}

void ThemeEditorDialog::loadPersisted(const QString& categoryKey, int& fore, int& back) const
{
    QSettings s;
    s.beginGroup(QStringLiteral("ThemeEditor/") + m_lexerName + QStringLiteral("/") + categoryKey);
    if (s.contains(QStringLiteral("fore"))) fore = s.value(QStringLiteral("fore")).toInt();
    if (s.contains(QStringLiteral("back"))) back = s.value(QStringLiteral("back")).toInt();
    s.endGroup();
}
