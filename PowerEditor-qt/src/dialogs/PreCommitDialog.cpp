#include "PreCommitDialog.h"

#include "../GitOps.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
constexpr const char* kSkeleton =
    "#!/usr/bin/env bash\n"
    "set -e\n"
    "# Pre-commit hook gerado pelo Notepad++ Qt.\n"
    "# Edite à vontade — o hook é só um shell script.\n"
    "\n"
    "files=$(git diff --cached --name-only --diff-filter=ACM)\n"
    "[ -z \"$files\" ] && exit 0\n"
    "\n";

constexpr const char* kClangFormat =
    "# clang-format\n"
    "echo \"$files\" | grep -E '\\.(c|cc|cpp|cxx|h|hh|hpp)$' | while read f; do\n"
    "    clang-format -i \"$f\" && git add \"$f\"\n"
    "done\n";

constexpr const char* kBlack =
    "# black\n"
    "py_files=$(echo \"$files\" | grep -E '\\.py$')\n"
    "[ -n \"$py_files\" ] && black -q $py_files && git add $py_files\n";

constexpr const char* kPrettier =
    "# prettier (js/ts/json/yaml/md/css/html)\n"
    "fmt_files=$(echo \"$files\" | grep -E '\\.(js|jsx|ts|tsx|json|ya?ml|md|css|html)$')\n"
    "[ -n \"$fmt_files\" ] && npx --no-install prettier -w $fmt_files && git add $fmt_files\n";

constexpr const char* kGofmt =
    "# gofmt\n"
    "go_files=$(echo \"$files\" | grep -E '\\.go$')\n"
    "[ -n \"$go_files\" ] && gofmt -w $go_files && git add $go_files\n";
}

PreCommitDialog::PreCommitDialog(const QString& anchorFilePath, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Pre-commit hook"));
    resize(720, 520);

    m_repoRoot = GitOps::repoRootFor(anchorFilePath);
    if (m_repoRoot.isEmpty()) {
        QMessageBox::warning(this, tr("Hook"), tr("Sem repositório."));
        return;
    }
    m_hookPath = QDir(m_repoRoot).absoluteFilePath(QStringLiteral(".git/hooks/pre-commit"));

    m_clangFormat = new QCheckBox(tr("clang-format (C/C++)"), this);
    m_black       = new QCheckBox(tr("black (Python)"), this);
    m_prettier    = new QCheckBox(tr("prettier (JS/TS/JSON/YAML/MD/CSS/HTML)"), this);
    m_gofmt       = new QCheckBox(tr("gofmt (Go)"), this);
    m_clangFormat->setChecked(true);
    m_black->setChecked(true);
    m_prettier->setChecked(true);
    m_gofmt->setChecked(true);

    auto* addBtn = new QPushButton(tr("Adicionar blocos selecionados"), this);

    m_script = new QPlainTextEdit(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    m_script->setFont(mono);

    // If a hook already exists, load it; otherwise seed with the skeleton.
    QFile f(m_hookPath);
    if (f.exists() && f.open(QIODevice::ReadOnly)) {
        m_script->setPlainText(QString::fromUtf8(f.readAll()));
        f.close();
    } else {
        m_script->setPlainText(QString::fromUtf8(kSkeleton));
    }

    m_status = new QLabel(this);

    auto* row = new QHBoxLayout();
    row->addWidget(m_clangFormat);
    row->addWidget(m_black);
    row->addWidget(m_prettier);
    row->addWidget(m_gofmt);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Close, this);
    bb->button(QDialogButtonBox::Save)->setText(tr("Instalar"));

    auto* lay = new QVBoxLayout(this);
    lay->addWidget(new QLabel(m_hookPath, this));
    lay->addLayout(row);
    lay->addWidget(addBtn);
    lay->addWidget(m_script, 1);
    lay->addWidget(m_status);
    lay->addWidget(bb);

    connect(addBtn, &QPushButton::clicked, this, &PreCommitDialog::onAddBlock);
    connect(bb, &QDialogButtonBox::accepted, this, &PreCommitDialog::onInstall);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void PreCommitDialog::onAddBlock()
{
    QString s = m_script->toPlainText();
    if (!s.endsWith('\n')) s += '\n';
    if (m_clangFormat->isChecked()) { s += '\n'; s += QString::fromUtf8(kClangFormat); }
    if (m_black->isChecked())       { s += '\n'; s += QString::fromUtf8(kBlack); }
    if (m_prettier->isChecked())    { s += '\n'; s += QString::fromUtf8(kPrettier); }
    if (m_gofmt->isChecked())       { s += '\n'; s += QString::fromUtf8(kGofmt); }
    m_script->setPlainText(s);
    m_status->setText(tr("Blocos inseridos. Revise e clique Instalar."));
}

void PreCommitDialog::onInstall()
{
    QFile f(m_hookPath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("Hook"), tr("Falha ao escrever %1").arg(m_hookPath));
        return;
    }
    f.write(m_script->toPlainText().toUtf8());
    f.close();
    // Make executable (rw-r--r-- → rwxr-xr-x).
    f.setPermissions(QFile::ReadOwner  | QFile::WriteOwner  | QFile::ExeOwner  |
                     QFile::ReadGroup  | QFile::ExeGroup    |
                     QFile::ReadOther  | QFile::ExeOther);
    QMessageBox::information(this, tr("Hook"),
        tr("Hook instalado em %1\nFica ativo na próxima vez que rodar `git commit`.").arg(m_hookPath));
    accept();
}
