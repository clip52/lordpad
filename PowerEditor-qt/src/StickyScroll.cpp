#include "StickyScroll.h"

#include <QEvent>
#include <QFont>
#include <QFontMetrics>
#include <QLabel>

#include <limits>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QSettings>
#include <QWidget>

#include <ScintillaEdit.h>

namespace {
constexpr const char* kEnabledKey = "StickyScroll/Enabled";

// Mask out the header flag (0x2000) and the level-numbered low bits.
constexpr int kFoldLevelHeaderFlag = 0x2000;
constexpr int kFoldLevelMask       = 0x0FFF;
} // namespace

StickyScroll::StickyScroll(QObject* parent) : QObject(parent)
{
    QSettings s;
    m_enabled = s.value(kEnabledKey, true).toBool();
}

void StickyScroll::setEnabled(bool on)
{
    if (m_enabled == on) return;
    m_enabled = on;
    QSettings s; s.setValue(kEnabledKey, on);
    if (!on) clearLabels();
    else     rebuildLabels();
}

void StickyScroll::setMaxLevels(int n)
{
    m_maxLevels = qBound(1, n, 12);
    rebuildLabels();
}

void StickyScroll::setActiveEditor(ScintillaEdit* editor)
{
    if (m_editor == editor) return;

    if (m_editor) {
        disconnect(m_editor.data(), nullptr, this, nullptr);
        if (m_editor->viewport()) m_editor->viewport()->removeEventFilter(this);
        clearLabels();
    }
    m_editor = editor;
    if (!m_editor) return;

    connect(m_editor.data(), &ScintillaEditBase::updateUi,
            this, [this](Scintilla::Update u) { onUpdateUi(static_cast<int>(u)); });
    if (m_editor->viewport()) m_editor->viewport()->installEventFilter(this);

    rebuildLabels();
}

void StickyScroll::onUpdateUi(int /*updated*/)
{
    rebuildLabels();
}

void StickyScroll::onEditorResized()
{
    layoutLabels();
}

QList<int> StickyScroll::activeHeaderLines() const
{
    if (!m_editor) return {};
    const int firstVisVisual = static_cast<int>(m_editor->firstVisibleLine());
    const int firstDoc       = static_cast<int>(m_editor->docLineFromVisible(firstVisVisual));

    QList<int> headers;
    if (firstDoc <= 0) return headers;

    int currentLine = firstDoc - 1;
    int lastLevel = std::numeric_limits<int>::max();
    while (currentLine >= 0 && headers.size() < m_maxLevels) {
        const int rawLevel = static_cast<int>(m_editor->foldLevel(currentLine));
        if (rawLevel & kFoldLevelHeaderFlag) {
            const int lvl = rawLevel & kFoldLevelMask;
            if (lvl < lastLevel) {
                // Only include this header if its fold spans the first visible line.
                const int last = static_cast<int>(m_editor->lastChild(currentLine, -1));
                if (last >= firstDoc) {
                    headers.prepend(currentLine);
                    lastLevel = lvl;
                }
            }
        }
        --currentLine;
    }
    return headers;
}

void StickyScroll::rebuildLabels()
{
    if (!m_editor || !m_enabled) { clearLabels(); return; }
    const QList<int> headers = activeHeaderLines();
    if (headers.isEmpty()) { clearLabels(); return; }

    QWidget* host = m_editor->viewport();
    if (!host) return;

    // Match label count to header count.
    while (m_labels.size() > headers.size()) {
        QLabel* l = m_labels.takeLast();
        l->deleteLater();
    }
    while (m_labels.size() < headers.size()) {
        auto* l = new QLabel(host);
        l->setAttribute(Qt::WA_TransparentForMouseEvents, false);
        l->setContentsMargins(8, 1, 8, 1);
        l->setStyleSheet(QStringLiteral(
            "QLabel { background: rgba(40,40,40,0.92); color: #d8d8d8; "
            "border-bottom: 1px solid #444; }"));
        l->setCursor(Qt::PointingHandCursor);
        l->installEventFilter(this);
        l->show();
        m_labels.append(l);
    }

    QFont mono = m_editor->font();
    QFontMetrics fm(mono);
    for (int i = 0; i < headers.size(); ++i) {
        QByteArray bytes = m_editor->getLine(headers[i]);
        QString line = QString::fromUtf8(bytes);
        while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);
        m_labels[i]->setText(line);
        m_labels[i]->setFont(mono);
        m_labels[i]->setProperty("docLine", headers[i]);
    }
    layoutLabels();
}

void StickyScroll::layoutLabels()
{
    if (!m_editor) return;
    QWidget* host = m_editor->viewport();
    if (!host) return;
    const int width = host->width();
    QFont mono = m_editor->font();
    QFontMetrics fm(mono);
    const int rowH = fm.height() + 2;
    for (int i = 0; i < m_labels.size(); ++i) {
        m_labels[i]->setGeometry(0, i * rowH, width, rowH);
        m_labels[i]->raise();
    }
}

void StickyScroll::clearLabels()
{
    for (QLabel* l : m_labels) l->deleteLater();
    m_labels.clear();
}

bool StickyScroll::eventFilter(QObject* watched, QEvent* event)
{
    Q_UNUSED(this);   // silence -Wunused-this in some builds
    if (m_editor && watched == m_editor->viewport()
        && event->type() == QEvent::Resize) {
        layoutLabels();
        return false;
    }
    if (auto* l = qobject_cast<QLabel*>(watched);
        l && event->type() == QEvent::MouseButtonRelease) {
        const int line = l->property("docLine").toInt();
        if (m_editor && line >= 0) {
            m_editor->gotoLine(line);
            m_editor->scrollCaret();
        }
        return true;
    }
    return QObject::eventFilter(watched, event);
}
