#include "ArchiveBrowserPanel.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSplitter>
#include <QStandardPaths>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

ArchiveBrowserPanel::ArchiveBrowserPanel(QWidget* parent)
    : QDockWidget(tr("Archive"), parent)
{
    setObjectName(QStringLiteral("ArchiveBrowserPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    m_openBtn = new QPushButton(tr("Abrir .zip / .tar…"), root);
    m_tree = new QTreeWidget(root);
    m_tree->setHeaderLabels({ tr("Path"), tr("Size") });
    m_tree->setRootIsDecorated(false);
    m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    m_preview = new QPlainTextEdit(root);
    m_preview->setReadOnly(true);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_preview->setFont(mono);

    m_status = new QLabel(root);

    auto* split = new QSplitter(Qt::Vertical, root);
    split->addWidget(m_tree);
    split->addWidget(m_preview);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 2);

    auto* row = new QHBoxLayout();
    row->addWidget(m_openBtn);
    row->addStretch(1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row);
    lay->addWidget(split, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_openBtn, &QPushButton::clicked, this, &ArchiveBrowserPanel::onOpen);
    connect(m_tree,    &QTreeWidget::itemActivated, this, &ArchiveBrowserPanel::onItemActivated);
}

ArchiveBrowserPanel::Kind ArchiveBrowserPanel::detectKind(const QString& path) const
{
    const QString lower = path.toLower();
    if (lower.endsWith(QStringLiteral(".zip"))) return Kind::Zip;
    if (lower.endsWith(QStringLiteral(".tar")) || lower.endsWith(QStringLiteral(".tar.gz"))
        || lower.endsWith(QStringLiteral(".tgz")) || lower.endsWith(QStringLiteral(".tar.bz2"))
        || lower.endsWith(QStringLiteral(".tar.xz")) || lower.endsWith(QStringLiteral(".tar.zst")))
        return Kind::Tar;
    return Kind::None;
}

void ArchiveBrowserPanel::onOpen()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Abrir archive"),
        QString(), tr("Archives (*.zip *.tar *.tar.gz *.tgz *.tar.bz2 *.tar.xz *.tar.zst);;Todos (*)"));
    if (path.isEmpty()) return;
    m_path = path;
    m_kind = detectKind(path);
    if (m_kind == Kind::None) { m_status->setText(tr("Tipo desconhecido.")); return; }
    rebuild();
}

void ArchiveBrowserPanel::rebuild()
{
    m_tree->clear();
    m_preview->clear();
    QString exe;
    QStringList args;
    if (m_kind == Kind::Zip) {
        exe = QStandardPaths::findExecutable(QStringLiteral("unzip"));
        if (exe.isEmpty()) { m_status->setText(tr("`unzip` não encontrado.")); return; }
        args = { QStringLiteral("-l"), m_path };
    } else {
        exe = QStandardPaths::findExecutable(QStringLiteral("tar"));
        if (exe.isEmpty()) { m_status->setText(tr("`tar` não encontrado.")); return; }
        args = { QStringLiteral("-tvf"), m_path };
    }
    QProcess p;
    p.start(exe, args);
    if (!p.waitForFinished(15000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }
    const QStringList lines = QString::fromUtf8(p.readAllStandardOutput()).split('\n');
    int count = 0;
    for (const QString& raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty()) continue;
        // Header / separator / footer of unzip: skip.
        if (line.startsWith(QStringLiteral("Archive:")) || line.startsWith(QStringLiteral("Length"))
            || line.startsWith(QStringLiteral("---"))) continue;
        QStringList parts = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (parts.size() < 2) continue;
        QString sizeStr, name;
        if (m_kind == Kind::Zip) {
            // "  size   date   time   name"
            sizeStr = parts[0];
            name = parts.mid(3).join(' ');
        } else {
            // "perms user/group size date time name"
            if (parts.size() < 6) continue;
            sizeStr = parts[2];
            name = parts.mid(5).join(' ');
        }
        if (name.isEmpty()) continue;
        auto* it = new QTreeWidgetItem(m_tree);
        it->setText(0, name);
        it->setText(1, sizeStr);
        ++count;
    }
    m_status->setText(tr("%1 entradas").arg(count));
}

void ArchiveBrowserPanel::onItemActivated(QTreeWidgetItem* it, int)
{
    if (!it || m_path.isEmpty()) return;
    const QString name = it->text(0);
    QString exe; QStringList args;
    if (m_kind == Kind::Zip) {
        exe = QStandardPaths::findExecutable(QStringLiteral("unzip"));
        args = { QStringLiteral("-p"), m_path, name };
    } else {
        exe = QStandardPaths::findExecutable(QStringLiteral("tar"));
        args = { QStringLiteral("-xOf"), m_path, name };
    }
    QProcess p;
    p.start(exe, args);
    if (!p.waitForFinished(15000)) { p.kill(); m_preview->setPlainText(tr("Timeout.")); return; }
    const QByteArray bytes = p.readAllStandardOutput();
    // Heuristic: don't dump binary into the preview.
    if (bytes.size() > 1 << 20) {
        m_preview->setPlainText(tr("(arquivo muito grande — %1 bytes)").arg(bytes.size()));
        return;
    }
    bool seemsBinary = false;
    for (int i = 0, n = qMin<int>(bytes.size(), 4096); i < n; ++i) {
        if (bytes[i] == 0) { seemsBinary = true; break; }
    }
    if (seemsBinary) m_preview->setPlainText(tr("(binário — %1 bytes)").arg(bytes.size()));
    else             m_preview->setPlainText(QString::fromUtf8(bytes));
}
