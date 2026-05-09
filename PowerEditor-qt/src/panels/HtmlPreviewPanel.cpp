#include "HtmlPreviewPanel.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextBrowser>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <ScintillaEdit.h>

HtmlPreviewPanel::HtmlPreviewPanel(QWidget* parent) : QDockWidget(tr("HTML"), parent)
{
    setObjectName(QStringLiteral("HtmlPreviewPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    m_view = new QTextBrowser(root);
    m_view->setOpenExternalLinks(true);
    m_refreshBtn = new QPushButton(tr("Atualizar"), root);
    m_liveBox = new QCheckBox(tr("ao vivo"), root); m_liveBox->setChecked(true);
    m_status = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(m_refreshBtn);
    row->addWidget(m_liveBox);
    row->addStretch(1);
    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row);
    lay->addWidget(m_view, 1);
    lay->addWidget(m_status);
    setWidget(root);

    m_debounce = new QTimer(this);
    m_debounce->setSingleShot(true); m_debounce->setInterval(300);
    connect(m_debounce, &QTimer::timeout, this, &HtmlPreviewPanel::refresh);
    connect(m_refreshBtn, &QPushButton::clicked, this, &HtmlPreviewPanel::refresh);
}

void HtmlPreviewPanel::setActiveEditor(ScintillaEdit* editor)
{
    if (m_editor) disconnect(m_editor.data(), nullptr, this, nullptr);
    m_editor = editor;
    if (m_editor)
        connect(m_editor.data(), &ScintillaEdit::modified, this, &HtmlPreviewPanel::onModified);
    refresh();
}

void HtmlPreviewPanel::onModified()
{
    if (m_liveBox->isChecked()) m_debounce->start();
}

void HtmlPreviewPanel::refresh()
{
    if (!m_editor) { m_view->setHtml(QStringLiteral("")); return; }
    QByteArray bytes = m_editor->getText(m_editor->textLength() + 1);
    if (!bytes.isEmpty() && bytes.endsWith('\0')) bytes.chop(1);
    const QString html = QString::fromUtf8(bytes);
    m_view->setHtml(html);
    m_status->setText(tr("%1 chars").arg(html.size()));
}
