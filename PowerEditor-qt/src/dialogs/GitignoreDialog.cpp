#include "GitignoreDialog.h"

#include "../GitOps.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
struct Template { const char* name; const char* content; };
const Template kTemplates[] = {
    {"C/C++",      "*.o\n*.obj\n*.a\n*.so\n*.dylib\nbuild/\nbuild_*/\nCMakeCache.txt\nCMakeFiles/\ncompile_commands.json\n"},
    {"Python",     "__pycache__/\n*.pyc\n.venv/\nvenv/\n.env\n.pytest_cache/\n.tox/\n.coverage\nhtmlcov/\n"},
    {"Node",       "node_modules/\nnpm-debug.log\n*.log\n.env\n.DS_Store\ndist/\ncoverage/\n.next/\n"},
    {"Rust",       "target/\n**/*.rs.bk\nCargo.lock\n.idea/\n"},
    {"Go",         "*.exe\n*.test\n*.out\nvendor/\nbin/\n"},
    {"Java",       "*.class\n*.jar\n*.war\ntarget/\n.idea/\n*.iml\n.gradle/\nbuild/\n"},
    {"IDE / OS",   ".idea/\n.vscode/\n*.swp\n.DS_Store\nThumbs.db\n.notepadpp/\n"},
};
}

GitignoreDialog::GitignoreDialog(const QString& anchorFilePath, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr(".gitignore"));
    resize(720, 480);

    const QString root = GitOps::repoRootFor(anchorFilePath);
    if (root.isEmpty()) {
        QMessageBox::warning(this, tr("gitignore"), tr("Sem repositório no path informado."));
        return;
    }
    m_path = QDir(root).absoluteFilePath(QStringLiteral(".gitignore"));

    m_text = new QPlainTextEdit(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_text->setFont(mono);
    QFile f(m_path);
    if (f.open(QIODevice::ReadOnly)) { m_text->setPlainText(QString::fromUtf8(f.readAll())); f.close(); }

    m_templates = new QListWidget(this);
    for (const Template& t : kTemplates) {
        auto* it = new QListWidgetItem(QString::fromUtf8(t.name), m_templates);
        it->setData(Qt::UserRole, QString::fromUtf8(t.content));
    }
    auto* insertBtn = new QPushButton(tr("Inserir template"), this);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close, this);

    auto* row = new QHBoxLayout();
    row->addWidget(new QLabel(tr("Templates:"), this));
    row->addWidget(m_templates, 1);
    row->addWidget(insertBtn);

    auto* lay = new QVBoxLayout(this);
    lay->addWidget(new QLabel(m_path, this));
    lay->addWidget(m_text, 1);
    lay->addLayout(row);
    lay->addWidget(bb);

    connect(bb, &QDialogButtonBox::accepted, this, &GitignoreDialog::onSave);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(insertBtn, &QPushButton::clicked, this, &GitignoreDialog::onInsertTemplate);
}

void GitignoreDialog::onSave()
{
    QFile f(m_path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("gitignore"), tr("Falha ao gravar %1").arg(m_path));
        return;
    }
    f.write(m_text->toPlainText().toUtf8());
    f.close();
    accept();
}

void GitignoreDialog::onInsertTemplate()
{
    auto* it = m_templates->currentItem();
    if (!it) return;
    QString content = m_text->toPlainText();
    if (!content.isEmpty() && !content.endsWith('\n')) content += '\n';
    content += QStringLiteral("\n# --- %1 ---\n").arg(it->text());
    content += it->data(Qt::UserRole).toString();
    m_text->setPlainText(content);
}
