#include "LspDiagnosticsPanel.h"

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QComboBox>
#include <QBrush>
#include <QColor>
#include <QFileInfo>

namespace {

QColor colorForSeverity(const QString& sev)
{
    if (sev == "error")   return QColor(220,  60,  60);
    if (sev == "warning") return QColor(220, 160,  40);
    if (sev == "info")    return QColor( 80, 140, 220);
    if (sev == "hint")    return QColor(140, 140, 140);
    return QColor(200, 200, 200);
}

QString translateSeverity(const QString& sev)
{
    if (sev == "error")   return LspDiagnosticsPanel::tr("Erro");
    if (sev == "warning") return LspDiagnosticsPanel::tr("Aviso");
    if (sev == "info")    return LspDiagnosticsPanel::tr("Info");
    if (sev == "hint")    return LspDiagnosticsPanel::tr("Dica");
    return sev;
}

int severityRank(const QString& sev)
{
    if (sev == "error")   return 0;
    if (sev == "warning") return 1;
    if (sev == "info")    return 2;
    if (sev == "hint")    return 3;
    return 4;
}

} // namespace

LspDiagnosticsPanel::LspDiagnosticsPanel(QWidget* parent)
    : QDockWidget(tr("Diagnósticos LSP"), parent)
{
    setObjectName(QStringLiteral("LspDiagnosticsPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    auto* vbox = new QVBoxLayout(root);
    vbox->setContentsMargins(4, 4, 4, 4);
    vbox->setSpacing(4);

    auto* topRow = new QHBoxLayout();
    topRow->setContentsMargins(0, 0, 0, 0);
    topRow->setSpacing(4);

    m_status = new QLabel(tr("Nenhum diagnóstico."), root);
    m_status->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_status->setWordWrap(true);

    m_filter = new QComboBox(root);
    m_filter->addItem(tr("Todos"));
    m_filter->addItem(tr("Erros + Avisos"));
    m_filter->addItem(tr("Somente erros"));
    m_filter->setToolTip(tr("Filtrar por severidade"));

    m_clearBtn = new QToolButton(root);
    m_clearBtn->setText(tr("Limpar"));
    m_clearBtn->setToolTip(tr("Limpar a lista de diagnósticos do arquivo atual"));

    topRow->addWidget(m_status, /*stretch=*/1);
    topRow->addWidget(m_filter);
    topRow->addWidget(m_clearBtn);
    vbox->addLayout(topRow);

    m_tree = new QTreeWidget(root);
    m_tree->setRootIsDecorated(false);
    m_tree->setUniformRowHeights(true);
    m_tree->setAllColumnsShowFocus(true);
    m_tree->setHeaderLabels(QStringList()
                            << tr("Severidade")
                            << tr("Linha")
                            << tr("Mensagem")
                            << tr("Origem"));
    m_tree->header()->setStretchLastSection(false);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_tree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_tree->setSortingEnabled(true);
    m_tree->sortByColumn(1, Qt::AscendingOrder);

    vbox->addWidget(m_tree, /*stretch=*/1);

    setWidget(root);

    connect(m_tree, &QTreeWidget::itemActivated,
            this,   &LspDiagnosticsPanel::onItemActivated);
    connect(m_tree, &QTreeWidget::itemDoubleClicked,
            this,   &LspDiagnosticsPanel::onItemActivated);
    connect(m_filter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,     &LspDiagnosticsPanel::onSeverityFilterChanged);
    connect(m_clearBtn, &QToolButton::clicked,
            this,       &LspDiagnosticsPanel::onClearClicked);
}

void LspDiagnosticsPanel::setDiagnostics(const QString& filePath, const QList<LspDiagnostic>& diags)
{
    if (filePath.isEmpty()) return;
    if (diags.isEmpty()) m_cache.remove(filePath);
    else                 m_cache.insert(filePath, diags);
    if (filePath == m_activeFile) rebuildTree();
}

void LspDiagnosticsPanel::setActiveFile(const QString& filePath)
{
    if (m_activeFile == filePath) return;
    m_activeFile = filePath;
    rebuildTree();
}

void LspDiagnosticsPanel::showStatusMessage(const QString& text)
{
    m_status->setText(text);
}

void LspDiagnosticsPanel::onSeverityFilterChanged(int index)
{
    m_severityFilter = index;
    rebuildTree();
}

void LspDiagnosticsPanel::onClearClicked()
{
    if (m_activeFile.isEmpty()) return;
    m_cache.remove(m_activeFile);
    rebuildTree();
}

void LspDiagnosticsPanel::onItemActivated(QTreeWidgetItem* item, int /*column*/)
{
    if (!item) return;
    const int    line = item->data(1, Qt::UserRole).toInt();
    const int    col  = item->data(2, Qt::UserRole).toInt();
    const QString fp  = item->data(0, Qt::UserRole).toString();
    if (fp.isEmpty()) return;
    emit diagnosticActivated(fp, line, col);
}

void LspDiagnosticsPanel::rebuildTree()
{
    m_tree->clear();

    if (m_activeFile.isEmpty()) {
        m_status->setText(tr("Nenhum arquivo ativo."));
        return;
    }

    const QList<LspDiagnostic> diags = m_cache.value(m_activeFile);
    if (diags.isEmpty()) {
        m_status->setText(tr("Sem diagnósticos para %1.").arg(QFileInfo(m_activeFile).fileName()));
        return;
    }

    int errors = 0, warnings = 0, infos = 0, hints = 0;
    m_tree->setSortingEnabled(false);
    for (const LspDiagnostic& d : diags) {
        if (d.severity == "error")   ++errors;
        else if (d.severity == "warning") ++warnings;
        else if (d.severity == "info")    ++infos;
        else if (d.severity == "hint")    ++hints;

        // Apply severity filter.
        if (m_severityFilter == 1 && (d.severity == "info" || d.severity == "hint"))
            continue;
        if (m_severityFilter == 2 && d.severity != "error")
            continue;

        auto* it = new QTreeWidgetItem();
        it->setText(0, translateSeverity(d.severity));
        it->setText(1, QString::number(d.line + 1)); // show 1-based
        QString msg = d.message;
        if (!d.code.isEmpty()) msg = QStringLiteral("[%1] %2").arg(d.code, msg);
        it->setText(2, msg);
        it->setText(3, d.source);

        const QColor c = colorForSeverity(d.severity);
        const QBrush brush(c);
        for (int i = 0; i < 4; ++i) it->setForeground(i, brush);

        // Stash navigation payload.
        it->setData(0, Qt::UserRole, m_activeFile);
        it->setData(1, Qt::UserRole, d.line);
        it->setData(2, Qt::UserRole, d.column);

        // Numeric sort key for severity column (so sort matches semantic order).
        it->setData(0, Qt::UserRole + 1, severityRank(d.severity));
        // Numeric sort key for line column.
        it->setData(1, Qt::UserRole + 1, d.line);

        m_tree->addTopLevelItem(it);
    }
    m_tree->setSortingEnabled(true);

    m_status->setText(tr("%1 — %2 erros, %3 avisos, %4 info, %5 dicas")
                      .arg(QFileInfo(m_activeFile).fileName())
                      .arg(errors).arg(warnings).arg(infos).arg(hints));
}
