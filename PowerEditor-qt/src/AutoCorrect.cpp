#include "AutoCorrect.h"

#include <QEvent>
#include <QKeyEvent>
#include <QSettings>

#include <ScintillaEdit.h>

namespace { constexpr const char* kEnabledKey = "AutoCorrect/Enabled"; constexpr const char* kDictKey = "AutoCorrect/dict"; }

AutoCorrect::AutoCorrect(QObject* parent) : QObject(parent)
{
    QSettings s; m_enabled = s.value(kEnabledKey, true).toBool();
    load();
    if (m_dict.isEmpty()) { seedDefaults(); persist(); }
}

void AutoCorrect::setEnabled(bool on)
{
    if (m_enabled == on) return;
    m_enabled = on;
    QSettings s; s.setValue(kEnabledKey, on);
}

void AutoCorrect::attach(ScintillaEdit* editor)
{
    if (!editor || m_editors.contains(editor)) return;
    m_editors.insert(editor);
    editor->installEventFilter(this);
    connect(editor, &QObject::destroyed, this, [this](QObject* obj) {
        m_editors.remove(reinterpret_cast<ScintillaEdit*>(obj));
    });
}
void AutoCorrect::detach(ScintillaEdit* editor)
{
    if (!editor) return;
    editor->removeEventFilter(this);
    m_editors.remove(editor);
}

void AutoCorrect::seedDefaults()
{
    // Erros comuns em PT-BR e EN-US.
    m_dict[QStringLiteral("nao")]      = QStringLiteral("não");
    m_dict[QStringLiteral("voce")]     = QStringLiteral("você");
    m_dict[QStringLiteral("entao")]    = QStringLiteral("então");
    m_dict[QStringLiteral("teh")]      = QStringLiteral("the");
    m_dict[QStringLiteral("recieve")]  = QStringLiteral("receive");
    m_dict[QStringLiteral("seperate")] = QStringLiteral("separate");
    m_dict[QStringLiteral("occured")]  = QStringLiteral("occurred");
    m_dict[QStringLiteral("definately")] = QStringLiteral("definitely");
    m_dict[QStringLiteral("alot")]     = QStringLiteral("a lot");
    m_dict[QStringLiteral("--")]       = QStringLiteral("—");   // em-dash
}

void AutoCorrect::load()
{
    QSettings s; s.beginGroup(kDictKey);
    for (const QString& k : s.allKeys()) m_dict[k] = s.value(k).toString();
    s.endGroup();
}
void AutoCorrect::persist() const
{
    QSettings s; s.beginGroup(kDictKey);
    s.remove(QString());
    for (auto it = m_dict.constBegin(); it != m_dict.constEnd(); ++it)
        s.setValue(it.key(), it.value());
    s.endGroup();
}
void AutoCorrect::setEntry(const QString& wrong, const QString& right)
{
    if (wrong.isEmpty()) return;
    m_dict[wrong] = right;
    persist();
}
void AutoCorrect::removeEntry(const QString& wrong)
{
    m_dict.remove(wrong); persist();
}

bool AutoCorrect::eventFilter(QObject* watched, QEvent* event)
{
    if (!m_enabled) return QObject::eventFilter(watched, event);
    if (event->type() != QEvent::KeyPress) return QObject::eventFilter(watched, event);
    auto* sci = qobject_cast<ScintillaEdit*>(watched);
    if (!sci || !m_editors.contains(sci)) return QObject::eventFilter(watched, event);

    auto* ke = static_cast<QKeyEvent*>(event);
    // Trigger on space, enter, period, comma, semicolon — boundaries that
    // typically end a word.
    const QChar c = ke->text().isEmpty() ? QChar() : ke->text()[0];
    if (c == ' ' || c == '\n' || c == '\r' || c == '.' || c == ',' || c == ';' || c == '\t') {
        // Let the keystroke through, then correct *after* it lands so we know
        // the word boundary ended.
        QMetaObject::invokeMethod(this, [this, sci]() { tryCorrectAtCaret(sci); }, Qt::QueuedConnection);
    }
    return QObject::eventFilter(watched, event);
}

void AutoCorrect::tryCorrectAtCaret(ScintillaEdit* sci)
{
    if (!sci) return;
    const sptr_t pos = sci->currentPos();
    if (pos < 2) return;
    // Backwards: skip the boundary char we just inserted, then read word.
    sptr_t end = pos - 1;
    if (end <= 0) return;
    sptr_t start = sci->wordStartPosition(end, true);
    if (start >= end) return;
    const QString word = QString::fromUtf8(sci->textRange(start, end));
    const auto it = m_dict.constFind(word);
    if (it == m_dict.constEnd()) return;
    const QByteArray repl = it.value().toUtf8();
    sci->beginUndoAction();
    sci->setTargetRange(start, end);
    sci->replaceTarget(repl.size(), repl.constData());
    sci->endUndoAction();
}
