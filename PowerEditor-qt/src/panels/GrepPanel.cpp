#include "GrepPanel.h"

#include <QCheckBox>
#include <QDir>
#include <QFileDialog>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

GrepPanel::GrepPanel(QWidget* parent) : QDockWidget(tr("Grep"), parent)
{
    setObjectName(QStringLiteral("GrepPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_pattern = new QLineEdit(root); m_pattern->setPlaceholderText(QStringLiteral("padrão"));
    m_dir     = new QLineEdit(root); m_dir->setPlaceholderText(QStringLiteral("diretório"));
    m_pickBtn = new QPushButton(tr("…"), root);
    m_searchBtn = new QPushButton(tr("Buscar"), root);
    m_caseBox  = new QCheckBox(tr("case"), root);
    m_wordBox  = new QCheckBox(tr("palavra"), root);
    m_regexBox = new QCheckBox(tr("regex"), root);
    m_tree = new QTreeWidget(root);
    m_tree->setHeaderLabels({ tr("Arquivo"), tr("Linha"), tr("Texto") });
    m_tree->setRootIsDecorated(false);
    m_tree->setUniformRowHeights(true);
    m_tree->setFont(mono);
    m_status = new QLabel(root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("Padrão:"), root));
    row1->addWidget(m_pattern, 1);
    row1->addWidget(m_caseBox);
    row1->addWidget(m_wordBox);
    row1->addWidget(m_regexBox);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(new QLabel(tr("Dir:"), root));
    row2->addWidget(m_dir, 1);
    row2->addWidget(m_pickBtn);
    row2->addWidget(m_searchBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(m_tree, 1);
    lay->addWidget(m_status);
    setWidget(root);

    QSettings s;
    m_dir->setText(s.value(QStringLiteral("Grep/dir")).toString());

    connect(m_pickBtn, &QPushButton::clicked, this, &GrepPanel::onPickDir);
    connect(m_searchBtn, &QPushButton::clicked, this, &GrepPanel::onSearch);
    connect(m_pattern, &QLineEdit::returnPressed, this, &GrepPanel::onSearch);
    connect(m_tree, &QTreeWidget::itemActivated, this, &GrepPanel::onItemActivated);
}

void GrepPanel::onPickDir()
{
    const QString p = QFileDialog::getExistingDirectory(this, tr("Diretório"), m_dir->text());
    if (!p.isEmpty()) m_dir->setText(p);
}

void GrepPanel::onSearch()
{
    const QString rg = QStandardPaths::findExecutable(QStringLiteral("rg"));
    if (rg.isEmpty()) { m_status->setText(tr("ripgrep (rg) não encontrado no PATH.")); return; }

    const QString dir = m_dir->text().trimmed();
    if (dir.isEmpty() || !QDir(dir).exists()) { m_status->setText(tr("Diretório inválido.")); return; }
    QSettings().setValue(QStringLiteral("Grep/dir"), dir);

    if (m_proc) { m_proc->kill(); m_proc->deleteLater(); }
    m_proc = new QProcess(this);
    m_proc->setWorkingDirectory(dir);
    m_proc->setProcessChannelMode(QProcess::SeparateChannels);
    m_buf.clear();
    m_hits = 0;
    m_tree->clear();

    QStringList args = { QStringLiteral("--no-heading"), QStringLiteral("-n"),
                          QStringLiteral("--column"), QStringLiteral("-H"),
                          QStringLiteral("--max-count=200") };
    if (!m_caseBox->isChecked())  args << QStringLiteral("-i");
    if (m_wordBox->isChecked())   args << QStringLiteral("-w");
    if (!m_regexBox->isChecked()) args << QStringLiteral("-F");
    args << QStringLiteral("--") << m_pattern->text();

    connect(m_proc.data(), &QProcess::readyReadStandardOutput, this, &GrepPanel::onProcessOutput);
    connect(m_proc.data(), QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &GrepPanel::onProcessFinished);

    m_status->setText(tr("Buscando…"));
    m_proc->start(rg, args);
}

void GrepPanel::onProcessOutput()
{
    if (!m_proc) return;
    m_buf += m_proc->readAllStandardOutput();
    int nl;
    while ((nl = m_buf.indexOf('\n')) >= 0) {
        const QByteArray rawLine = m_buf.left(nl);
        m_buf.remove(0, nl + 1);
        const QString line = QString::fromUtf8(rawLine);
        // Format: file:line:col:text
        const int firstColon  = line.indexOf(':');
        const int secondColon = line.indexOf(':', firstColon + 1);
        const int thirdColon  = line.indexOf(':', secondColon + 1);
        if (firstColon < 0 || secondColon < 0 || thirdColon < 0) continue;
        const QString file = line.left(firstColon);
        const int    lnum = line.mid(firstColon + 1, secondColon - firstColon - 1).toInt();
        const QString text = line.mid(thirdColon + 1);

        auto* item = new QTreeWidgetItem(m_tree);
        item->setText(0, file);
        item->setText(1, QString::number(lnum));
        item->setText(2, text.trimmed().left(200));
        item->setData(0, Qt::UserRole, QDir(m_proc->workingDirectory()).absoluteFilePath(file));
        item->setData(0, Qt::UserRole + 1, lnum);
        ++m_hits;
        if (m_hits >= 5000) { m_proc->kill(); break; }
    }
}

void GrepPanel::onProcessFinished()
{
    for (int c = 0; c < 3; ++c) m_tree->resizeColumnToContents(c);
    m_status->setText(tr("%1 matches.").arg(m_hits));
}

void GrepPanel::onItemActivated()
{
    auto* it = m_tree->currentItem();
    if (!it) return;
    emit openFileAtLine(it->data(0, Qt::UserRole).toString(),
                        it->data(0, Qt::UserRole + 1).toInt());
}
