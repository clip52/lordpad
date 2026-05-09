#include "BracketColors.h"

#include <QSettings>
#include <QTimer>

#include <ScintillaEdit.h>

#include <vector>

namespace {

// Four well-spaced colors (Scintilla 0xBBGGRR). Same palette VS Code uses
// by default for bracket pair colorization, ordered to maximize contrast
// against typical dark + light backgrounds.
constexpr sptr_t kPalette[4] = {
    0x00DCB179,  // soft blue-violet
    0x0000C8C8,  // amber-yellow
    0x00C896C8,  // pink-magenta
    0x0064D2FF,  // sky-orange
};

constexpr const char* kEnabledKey = "BracketColors/Enabled";

// Heuristic: a buffer position is "inside a string-ish style" when
// Scintilla's per-position style number falls into a family commonly used
// by lexers for strings, chars, comments. We can't enumerate every lexer
// constant — instead match by name fragment of the style. We approximate by
// checking the StyleAt value against a small allow-list per common lexer
// (cpp 6/11, python 3/4/13, etc.). For unknown lexers the heuristic is
// "skip nothing" — colorization on lines inside strings is harmless.
bool styleIsString(int lexerLanguage, int style)
{
    switch (lexerLanguage) {
        case 3:    // SCLEX_CPP
            return style == 6 || style == 7 || style == 11 || style == 12 || style == 13;
        case 2:    // SCLEX_PYTHON
            return style == 3 || style == 4 || style == 6 || style == 13;
        case 7:    // SCLEX_HTML / generic
            return style == 6;
        default:
            return false;
    }
}

} // namespace

BracketColors::BracketColors(QObject* parent) : QObject(parent)
{
    QSettings s;
    m_enabled = s.value(kEnabledKey, true).toBool();
}

BracketColors& BracketColors::shared()
{
    static BracketColors* inst = nullptr;
    if (!inst) inst = new BracketColors();
    return *inst;
}

void BracketColors::installFor(ScintillaEdit* editor) { shared().attach(editor); }

void BracketColors::setEnabled(bool on)
{
    if (m_enabled == on) return;
    m_enabled = on;
    QSettings s; s.setValue(kEnabledKey, on);
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        if (!it->editor) continue;
        if (m_enabled) rescan(it->editor.data());
        else           clearIndicators(it->editor.data());
    }
}

void BracketColors::attach(ScintillaEdit* editor)
{
    if (!editor) return;
    if (m_entries.contains(editor)) return;

    Entry e;
    e.editor   = editor;
    e.debounce = new QTimer(this);
    e.debounce->setSingleShot(true);
    e.debounce->setInterval(200);
    connect(e.debounce, &QTimer::timeout, this, &BracketColors::onRescanTimeout);
    m_entries.insert(editor, e);

    configureIndicators(editor);
    connect(editor, &ScintillaEditBase::modified, this, &BracketColors::onModified);
    connect(editor, &QObject::destroyed, this, &BracketColors::onEditorDestroyed);

    if (m_enabled) rescan(editor);
}

void BracketColors::detach(ScintillaEdit* editor)
{
    auto it = m_entries.find(editor);
    if (it == m_entries.end()) return;
    if (editor) {
        disconnect(editor, nullptr, this, nullptr);
        clearIndicators(editor);
    }
    if (it->debounce) it->debounce->deleteLater();
    m_entries.erase(it);
}

void BracketColors::onEditorDestroyed(QObject* obj)
{
    auto it = m_entries.find(reinterpret_cast<ScintillaEdit*>(obj));
    if (it == m_entries.end()) return;
    if (it->debounce) it->debounce->deleteLater();
    m_entries.erase(it);
}

void BracketColors::onModified(Scintilla::ModificationFlags type,
                               Scintilla::Position /*position*/,
                               Scintilla::Position /*length*/,
                               Scintilla::Position /*linesAdded*/,
                               const QByteArray& /*text*/,
                               Scintilla::Position /*line*/,
                               Scintilla::FoldLevel /*foldNow*/,
                               Scintilla::FoldLevel /*foldPrev*/)
{
    if (!m_enabled) return;
    const auto mask = Scintilla::ModificationFlags::InsertText
                    | Scintilla::ModificationFlags::DeleteText;
    if (static_cast<int>(type & mask) == 0) return;

    auto* sci = qobject_cast<ScintillaEdit*>(sender());
    if (!sci) return;
    auto it = m_entries.find(sci);
    if (it == m_entries.end() || !it->debounce) return;
    it->debounce->start();
}

void BracketColors::onRescanTimeout()
{
    auto* timer = qobject_cast<QTimer*>(sender());
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        if (it->debounce == timer && it->editor) {
            rescan(it->editor.data());
            return;
        }
    }
}

void BracketColors::configureIndicators(ScintillaEdit* editor) const
{
    if (!editor) return;
    for (int i = 0; i < 4; ++i) {
        const int idx = kIndicatorBase + i;
        editor->indicSetStyle(idx, INDIC_TEXTFORE);
        editor->indicSetFore(idx, kPalette[i]);
    }
}

void BracketColors::clearIndicators(ScintillaEdit* editor) const
{
    if (!editor) return;
    const sptr_t end = editor->length();
    for (int i = 0; i < 4; ++i) {
        editor->setIndicatorCurrent(kIndicatorBase + i);
        editor->indicatorClearRange(0, end);
    }
}

void BracketColors::rescan(ScintillaEdit* editor) const
{
    if (!editor || !m_enabled) return;
    clearIndicators(editor);

    const int lexerLanguage = static_cast<int>(editor->lexer());
    const QByteArray buf = editor->getText(editor->textLength() + 1);
    const int n = buf.size();

    std::vector<int> stack;   // stores depth-modulo-4 chosen at the open bracket
    auto pushPair = [&](int pos, char c) {
        const int depth = static_cast<int>(stack.size()) % 4;
        const int indic = kIndicatorBase + depth;
        editor->setIndicatorCurrent(indic);
        editor->indicatorFillRange(pos, 1);
        stack.push_back(indic);
        Q_UNUSED(c);
    };
    auto popPair = [&](int pos) {
        if (stack.empty()) return;
        const int indic = stack.back();
        stack.pop_back();
        editor->setIndicatorCurrent(indic);
        editor->indicatorFillRange(pos, 1);
    };

    for (int i = 0; i < n; ++i) {
        const char c = buf[i];
        if (c != '(' && c != ')'
            && c != '[' && c != ']'
            && c != '{' && c != '}') continue;
        const int style = static_cast<int>(editor->styleAt(i));
        if (styleIsString(lexerLanguage, style)) continue;
        if (c == '(' || c == '[' || c == '{') pushPair(i, c);
        else                                  popPair(i);
    }
}
