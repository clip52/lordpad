#include "TodoAggregatorPanel.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QTextStream>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

TodoAggregatorPanel::TodoAggregatorPanel(QWidget* parent) : QDockWidget(tr("TODOs"), parent)
{
    setObjectName(QStringLiteral("TodoAggregatorPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    m_dirEdit = new QLineEdit(root);
    m_dirEdit->setPlaceholderText(QStringLiteral("Diretório a varrer"));
    m_pickBtn = new QPushButton(tr("…"), root);
    m_scanBtn = new QPushButton(tr("Escanear"), root);
    m_tree = new QTreeWidget(root);
    m_tree->setHeaderLabels({ tr("Marker"), tr("Arquivo"), tr("Linha"), tr("Texto") });
    m_tree->setRootIsDecorated(false);
    m_tree->setUniformRowHeights(true);
    m_status = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(new QLabel(tr("Dir:"), root));
    row->addWidget(m_dirEdit, 1);
    row->addWidget(m_pickBtn);
    row->addWidget(m_scanBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row);
    lay->addWidget(m_tree, 1);
    lay->addWidget(m_status);
    setWidget(root);

    QSettings s;
    m_dirEdit->setText(s.value(QStringLiteral("Todo/dir")).toString());

    connect(m_pickBtn, &QPushButton::clicked, this, &TodoAggregatorPanel::onPickDir);
    connect(m_scanBtn, &QPushButton::clicked, this, &TodoAggregatorPanel::onScan);
    connect(m_tree, &QTreeWidget::itemActivated, this, &TodoAggregatorPanel::onItemActivated);
}

void TodoAggregatorPanel::onPickDir()
{
    const QString p = QFileDialog::getExistingDirectory(this, tr("Diretório"), m_dirEdit->text());
    if (!p.isEmpty()) m_dirEdit->setText(p);
}

void TodoAggregatorPanel::onScan() { scan(m_dirEdit->text().trimmed()); }

void TodoAggregatorPanel::scan(const QString& dir)
{
    m_tree->clear();
    if (dir.isEmpty() || !QDir(dir).exists()) {
        m_status->setText(tr("Diretório inválido."));
        return;
    }
    QSettings().setValue(QStringLiteral("Todo/dir"), dir);

    QRegularExpression rx(QStringLiteral(R"((TODO|FIXME|XXX|HACK|NOTE|BUG)\b[:\s]?(.*))"),
                          QRegularExpression::CaseInsensitiveOption);
    int filesScanned = 0, hits = 0;
    QDirIterator it(dir, { QStringLiteral("*.cpp"), QStringLiteral("*.h"),
                            QStringLiteral("*.cc"), QStringLiteral("*.cxx"),
                            QStringLiteral("*.py"), QStringLiteral("*.js"),
                            QStringLiteral("*.ts"), QStringLiteral("*.tsx"),
                            QStringLiteral("*.go"), QStringLiteral("*.rs"),
                            QStringLiteral("*.rb"), QStringLiteral("*.java"),
                            QStringLiteral("*.kt"), QStringLiteral("*.cs"),
                            QStringLiteral("*.md"), QStringLiteral("*.sh"),
                            QStringLiteral("*.lua"), QStringLiteral("*.php"),
                            QStringLiteral("*.swift"), QStringLiteral("*.dart"),
                            QStringLiteral("*.html"), QStringLiteral("*.css") },
                  QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString p = it.next();
        if (p.contains(QStringLiteral("/.git/")) || p.contains(QStringLiteral("/node_modules/"))
         || p.contains(QStringLiteral("/build/")) || p.contains(QStringLiteral("/.venv/"))) continue;
        QFile f(p);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        QTextStream ts(&f);
        int ln = 0;
        while (!ts.atEnd()) {
            ++ln;
            const QString line = ts.readLine();
            auto m = rx.match(line);
            if (!m.hasMatch()) continue;
            auto* item = new QTreeWidgetItem(m_tree);
            item->setText(0, m.captured(1).toUpper());
            item->setText(1, QDir(dir).relativeFilePath(p));
            item->setText(2, QString::number(ln));
            item->setText(3, m.captured(2).trimmed().left(160));
            item->setData(0, Qt::UserRole, p);
            item->setData(0, Qt::UserRole + 1, ln);
            ++hits;
        }
        ++filesScanned;
    }
    for (int c = 0; c < 4; ++c) m_tree->resizeColumnToContents(c);
    m_status->setText(tr("%1 hits em %2 arquivos.").arg(hits).arg(filesScanned));
}

void TodoAggregatorPanel::onItemActivated()
{
    auto* it = m_tree->currentItem();
    if (!it) return;
    emit openFileAtLine(it->data(0, Qt::UserRole).toString(),
                        it->data(0, Qt::UserRole + 1).toInt());
}
