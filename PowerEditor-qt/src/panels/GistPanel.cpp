#include "GistPanel.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QVBoxLayout>
#include <QWidget>

#include <ScintillaEdit.h>

GistPanel::GistPanel(QWidget* parent) : QDockWidget(tr("Gist"), parent)
{
    setObjectName(QStringLiteral("GistPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_descEdit = new QLineEdit(root); m_descEdit->setPlaceholderText(tr("descrição"));
    m_filenameEdit = new QLineEdit(root);
    m_filenameEdit->setPlaceholderText(QStringLiteral("snippet.txt"));
    m_publicBox = new QCheckBox(tr("público"), root);
    m_selectionBox = new QCheckBox(tr("apenas seleção"), root);
    m_createBtn = new QPushButton(tr("Criar Gist"), root);
    m_listBtn   = new QPushButton(tr("Listar meus"), root);
    m_out = new QPlainTextEdit(root); m_out->setReadOnly(true); m_out->setFont(mono);
    m_status = new QLabel(root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("Desc:"), root));
    row1->addWidget(m_descEdit, 1);
    row1->addWidget(new QLabel(tr("Arquivo:"), root));
    row1->addWidget(m_filenameEdit);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(m_publicBox);
    row2->addWidget(m_selectionBox);
    row2->addStretch(1);
    row2->addWidget(m_listBtn);
    row2->addWidget(m_createBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(m_out, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_createBtn, &QPushButton::clicked, this, &GistPanel::onCreateGist);
    connect(m_listBtn,   &QPushButton::clicked, this, &GistPanel::onListGists);
}

void GistPanel::onCreateGist()
{
    if (!m_editor) { m_status->setText(tr("Sem editor.")); return; }
    const QString gh = QStandardPaths::findExecutable(QStringLiteral("gh"));
    if (gh.isEmpty()) { m_status->setText(tr("`gh` (GitHub CLI) não encontrado.")); return; }

    QByteArray content;
    if (m_selectionBox->isChecked()) content = m_editor->getSelText();
    else                              content = m_editor->getText(m_editor->textLength() + 1);
    if (!content.isEmpty() && content.endsWith('\0')) content.chop(1);
    if (content.isEmpty()) { m_status->setText(tr("Conteúdo vazio.")); return; }

    QTemporaryFile tmp(QStringLiteral("gist_XXXXXX_") +
                        (m_filenameEdit->text().isEmpty() ? QStringLiteral("snippet.txt")
                                                          : m_filenameEdit->text()));
    tmp.setAutoRemove(true);
    if (!tmp.open()) { m_status->setText(tr("Tmp falhou.")); return; }
    tmp.write(content);
    tmp.close();

    QStringList args = { QStringLiteral("gist"), QStringLiteral("create") };
    if (!m_descEdit->text().isEmpty())
        args << QStringLiteral("-d") << m_descEdit->text();
    if (m_publicBox->isChecked()) args << QStringLiteral("--public");
    args << tmp.fileName();

    QProcess p;
    p.setProcessChannelMode(QProcess::SeparateChannels);
    p.start(gh, args);
    if (!p.waitForFinished(20000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }
    const QString out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    const QString err = QString::fromUtf8(p.readAllStandardError()).trimmed();
    if (p.exitCode() == 0 && out.startsWith(QStringLiteral("https://"))) {
        m_out->appendPlainText(out);
        QApplication::clipboard()->setText(out);
        m_status->setText(tr("URL copiado."));
    } else {
        m_out->appendPlainText(err.isEmpty() ? out : err);
        m_status->setText(tr("Falhou."));
    }
}

void GistPanel::onListGists()
{
    const QString gh = QStandardPaths::findExecutable(QStringLiteral("gh"));
    if (gh.isEmpty()) { m_status->setText(tr("`gh` não encontrado.")); return; }
    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(gh, { QStringLiteral("gist"), QStringLiteral("list"),
                   QStringLiteral("-L"), QStringLiteral("30") });
    if (!p.waitForFinished(15000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }
    m_out->setPlainText(QString::fromUtf8(p.readAllStandardOutput()));
    m_status->setText(tr("Listado."));
}
