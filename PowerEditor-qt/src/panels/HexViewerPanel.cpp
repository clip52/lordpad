#include "HexViewerPanel.h"

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

#include <ScintillaEdit.h>

HexViewerPanel::HexViewerPanel(QWidget* parent) : QDockWidget(tr("Hex"), parent)
{
    setObjectName(QStringLiteral("HexViewerPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_perRow = new QSpinBox(root); m_perRow->setRange(4, 64); m_perRow->setValue(16);
    m_refreshBtn = new QPushButton(tr("Atualizar"), root);
    m_view = new QPlainTextEdit(root); m_view->setReadOnly(true); m_view->setFont(mono);
    m_view->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_status = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(new QLabel(tr("Bytes/linha:"), root)); row->addWidget(m_perRow);
    row->addWidget(m_refreshBtn);
    row->addStretch(1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row);
    lay->addWidget(m_view, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_refreshBtn, &QPushButton::clicked, this, &HexViewerPanel::refresh);
}

void HexViewerPanel::setActiveEditor(ScintillaEdit* editor) { m_editor = editor; refresh(); }

QString HexViewerPanel::render(const QByteArray& bytes, int bytesPerRow) const
{
    QString out;
    out.reserve(bytes.size() * 4);
    for (int off = 0; off < bytes.size(); off += bytesPerRow) {
        QString hex, ascii;
        const int end = qMin(off + bytesPerRow, bytes.size());
        for (int i = off; i < end; ++i) {
            const unsigned char b = static_cast<unsigned char>(bytes[i]);
            hex += QString::asprintf("%02x ", b);
            ascii += (b >= 0x20 && b < 0x7f) ? QChar(b) : QChar('.');
        }
        // pad hex to fixed width when last row is short.
        const int pad = bytesPerRow - (end - off);
        for (int i = 0; i < pad; ++i) hex += "   ";
        out += QString::asprintf("%08x  ", off) + hex + " " + ascii + "\n";
    }
    return out;
}

void HexViewerPanel::refresh()
{
    if (!m_editor) { m_view->clear(); m_status->setText(tr("Sem editor.")); return; }
    QByteArray bytes = m_editor->getText(m_editor->textLength() + 1);
    if (!bytes.isEmpty() && bytes.endsWith('\0')) bytes.chop(1);
    if (bytes.size() > 1 << 20) {
        m_status->setText(tr("Buffer >1MB; mostrando primeiro 1MB."));
        bytes = bytes.left(1 << 20);
    } else {
        m_status->setText(tr("%1 bytes").arg(bytes.size()));
    }
    m_view->setPlainText(render(bytes, m_perRow->value()));
}
