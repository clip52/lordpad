#include "Snippets.h"

#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QChar>
#include <QRegularExpression>

#include <algorithm>
#include <limits>

#include "ScintillaEdit.h"

namespace {

constexpr const char* kKeyAll         = "snippets/all";
constexpr const char* kKeyInitialised = "snippets/initialised";

inline bool isWordChar(QChar c) {
    return c.isLetterOrNumber() || c == QLatin1Char('_');
}

// One placeholder slot found in the snippet body. byteStart / byteLen are
// relative to the *expanded* UTF-8 string and point at the default text
// (or a 0-length insertion when the placeholder had no default).
struct ParsedStop {
    int number   = 0;
    int byteStart = 0;
    int byteLen   = 0;
};

// Expand placeholder syntax and return both the expanded text *and* a list
// of stops in source order. Source order matters because ${1} can appear
// after ${2} in the body — we still want to navigate ${1}, ${2}, ..., ${0}.
QString expandPlaceholders(const QString& body, QList<ParsedStop>& outStops) {
    outStops.clear();
    QString out;
    out.reserve(body.size());

    auto emitStop = [&](int n, const QString& defaultText) {
        ParsedStop s;
        s.number    = n;
        s.byteStart = out.toUtf8().size();
        out.append(defaultText);
        s.byteLen   = out.toUtf8().size() - s.byteStart;
        outStops.append(s);
    };

    int i = 0;
    const int n = body.size();
    while (i < n) {
        const QChar c = body.at(i);
        if (c == QLatin1Char('$') && i + 1 < n && body.at(i + 1) == QLatin1Char('{')) {
            int j = i + 2;
            const int numStart = j;
            while (j < n && body.at(j).isDigit()) ++j;
            if (j > numStart && j < n) {
                bool ok = false;
                const int num = body.mid(numStart, j - numStart).toInt(&ok);
                if (ok && body.at(j) == QLatin1Char('}')) {
                    emitStop(num, QString());
                    i = j + 1;
                    continue;
                } else if (ok && body.at(j) == QLatin1Char(':')) {
                    int k = j + 1;
                    int depth = 1;
                    QString def;
                    while (k < n && depth > 0) {
                        const QChar ck = body.at(k);
                        if (ck == QLatin1Char('{')) {
                            ++depth; def.append(ck); ++k;
                        } else if (ck == QLatin1Char('}')) {
                            --depth;
                            if (depth == 0) break;
                            def.append(ck); ++k;
                        } else { def.append(ck); ++k; }
                    }
                    if (k < n && body.at(k) == QLatin1Char('}')) {
                        emitStop(num, def);
                        i = k + 1;
                        continue;
                    }
                }
            }
        }
        // Bare $N (no braces) — also a valid LSP-style tabstop ref.
        if (c == QLatin1Char('$') && i + 1 < n && body.at(i + 1).isDigit()) {
            int j = i + 1;
            while (j < n && body.at(j).isDigit()) ++j;
            const int num = body.mid(i + 1, j - i - 1).toInt();
            emitStop(num, QString());
            i = j;
            continue;
        }
        out.append(c);
        ++i;
    }
    return out;
}

} // namespace

Snippets::Snippets(QObject* parent)
    : QObject(parent) {
    load();
    QSettings s;
    if (!s.value(kKeyInitialised, false).toBool()) {
        seedDefaults();
        s.setValue(kKeyInitialised, true);
        s.sync();
        save();
    }
}

void Snippets::seedDefaults() {
    auto add = [&](const QString& lang, const QString& trigger,
                   const QString& body, const QString& desc) {
        Snippet s;
        s.trigger = trigger;
        s.body = body;
        s.description = desc;
        m_buckets[lang][trigger] = s;
    };

    // Global
    add(QString(), QStringLiteral("lorem"),
        QStringLiteral("Lorem ipsum dolor sit amet, consectetur adipiscing elit."),
        QStringLiteral("Lorem ipsum filler text"));

    // C++
    add(QStringLiteral("cpp"), QStringLiteral("for"),
        QStringLiteral("for (int i = 0; i < ${1:n}; ++i) {\n    ${2}\n}"),
        QStringLiteral("for loop"));
    add(QStringLiteral("cpp"), QStringLiteral("main"),
        QStringLiteral("int main(int argc, char* argv[]) {\n    ${1}\n    return 0;\n}"),
        QStringLiteral("main function"));
    add(QStringLiteral("cpp"), QStringLiteral("if"),
        QStringLiteral("if (${1:cond}) {\n    ${2}\n}"),
        QStringLiteral("if statement"));
    add(QStringLiteral("cpp"), QStringLiteral("include"),
        QStringLiteral("#include <${1:header}>"),
        QStringLiteral("#include directive"));

    // Python
    add(QStringLiteral("python"), QStringLiteral("for"),
        QStringLiteral("for ${1:i} in ${2:iterable}:\n    ${3}"),
        QStringLiteral("for loop"));
    add(QStringLiteral("python"), QStringLiteral("def"),
        QStringLiteral("def ${1:name}(${2}):\n    ${3:pass}"),
        QStringLiteral("function definition"));
    add(QStringLiteral("python"), QStringLiteral("class"),
        QStringLiteral("class ${1:Name}:\n    def __init__(self${2}):\n        ${3:pass}"),
        QStringLiteral("class definition"));

    // JavaScript
    add(QStringLiteral("javascript"), QStringLiteral("for"),
        QStringLiteral("for (let i = 0; i < ${1:n}; i++) {\n    ${2}\n}"),
        QStringLiteral("for loop"));
    add(QStringLiteral("javascript"), QStringLiteral("func"),
        QStringLiteral("function ${1:name}(${2}) {\n    ${3}\n}"),
        QStringLiteral("function declaration"));
    add(QStringLiteral("javascript"), QStringLiteral("log"),
        QStringLiteral("console.log(${1});"),
        QStringLiteral("console.log call"));
}

void Snippets::load() {
    m_buckets.clear();
    QSettings s;
    const QByteArray raw = s.value(kKeyAll).toByteArray();
    if (raw.isEmpty())
        return;

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray())
        return;

    const QJsonArray arr = doc.array();
    for (const QJsonValue& v : arr) {
        if (!v.isObject()) continue;
        const QJsonObject o = v.toObject();
        const QString lang    = o.value(QStringLiteral("lang")).toString();
        const QString trigger = o.value(QStringLiteral("trigger")).toString();
        if (trigger.isEmpty())
            continue;
        Snippet sn;
        sn.trigger     = trigger;
        sn.body        = o.value(QStringLiteral("body")).toString();
        sn.description = o.value(QStringLiteral("description")).toString();
        m_buckets[lang][trigger] = sn;
    }
}

void Snippets::save() const {
    QJsonArray arr;
    for (auto it = m_buckets.constBegin(); it != m_buckets.constEnd(); ++it) {
        const QString& lang = it.key();
        const auto& triggers = it.value();
        for (auto jt = triggers.constBegin(); jt != triggers.constEnd(); ++jt) {
            const Snippet& sn = jt.value();
            QJsonObject o;
            o.insert(QStringLiteral("lang"), lang);
            o.insert(QStringLiteral("trigger"), sn.trigger);
            o.insert(QStringLiteral("body"), sn.body);
            o.insert(QStringLiteral("description"), sn.description);
            arr.append(o);
        }
    }
    QSettings s;
    s.setValue(kKeyAll, QJsonDocument(arr).toJson(QJsonDocument::Compact));
    s.sync();
}

QList<Snippet> Snippets::forLanguage(const QString& lexerName) const {
    QList<Snippet> out;
    auto it = m_buckets.constFind(lexerName);
    if (it == m_buckets.constEnd())
        return out;
    out.reserve(it.value().size());
    for (auto jt = it.value().constBegin(); jt != it.value().constEnd(); ++jt)
        out.append(jt.value());
    std::sort(out.begin(), out.end(), [](const Snippet& a, const Snippet& b) {
        return a.trigger.compare(b.trigger, Qt::CaseInsensitive) < 0;
    });
    return out;
}

void Snippets::upsert(const QString& lexerName, const Snippet& s) {
    if (s.trigger.isEmpty())
        return;
    m_buckets[lexerName][s.trigger] = s;
    save();
    emit changed();
}

void Snippets::remove(const QString& lexerName, const QString& trigger) {
    auto it = m_buckets.find(lexerName);
    if (it == m_buckets.end())
        return;
    if (it.value().remove(trigger) > 0) {
        if (it.value().isEmpty())
            m_buckets.erase(it);
        save();
        emit changed();
    }
}

QStringList Snippets::allLanguages() const {
    QStringList list = m_buckets.keys();
    std::sort(list.begin(), list.end(), [](const QString& a, const QString& b) {
        // Empty (global) first.
        if (a.isEmpty() && !b.isEmpty()) return true;
        if (!a.isEmpty() && b.isEmpty()) return false;
        return a.compare(b, Qt::CaseInsensitive) < 0;
    });
    return list;
}

bool Snippets::tryExpand(ScintillaEdit* editor, const QString& currentLexerName) {
    if (!editor)
        return false;

    const sptr_t caret = editor->currentPos();
    if (caret <= 0)
        return false;

    // Walk back over word chars in UTF-8 byte space using ScintillaEdit API.
    // We re-validate against our own definition of word chars by decoding the
    // candidate range and trimming.
    const sptr_t wordStart = editor->wordStartPosition(caret, /*onlyWordCharacters=*/true);
    if (wordStart >= caret)
        return false;

    QByteArray rangeBytes = editor->textRange(static_cast<int>(wordStart),
                                              static_cast<int>(caret));
    if (rangeBytes.isEmpty())
        return false;

    QString triggerWord = QString::fromUtf8(rangeBytes);
    // Trim to the trailing contiguous run of word chars (defensive).
    int start = triggerWord.size();
    while (start > 0 && isWordChar(triggerWord.at(start - 1)))
        --start;
    triggerWord = triggerWord.mid(start);
    if (triggerWord.isEmpty())
        return false;

    // The number of UTF-8 bytes corresponding to the actual trigger word.
    const int triggerByteLen = triggerWord.toUtf8().size();
    const sptr_t replaceStart = caret - triggerByteLen;

    // Find the snippet: language-specific first, then global.
    auto findSnippet = [&](const QString& lang, Snippet& out) -> bool {
        auto it = m_buckets.constFind(lang);
        if (it == m_buckets.constEnd())
            return false;
        auto jt = it.value().constFind(triggerWord);
        if (jt == it.value().constEnd())
            return false;
        out = jt.value();
        return true;
    };

    Snippet sn;
    if (!findSnippet(currentLexerName, sn)) {
        if (currentLexerName.isEmpty() || !findSnippet(QString(), sn))
            return false;
    }

    QList<ParsedStop> parsed;
    const QString expanded = expandPlaceholders(sn.body, parsed);
    const QByteArray expandedUtf8 = expanded.toUtf8();

    // Cancel any prior session before we start a new one.
    endSession();

    editor->beginUndoAction();
    editor->setTargetRange(static_cast<sptr_t>(replaceStart),
                           static_cast<sptr_t>(caret));
    editor->replaceTarget(static_cast<sptr_t>(expandedUtf8.size()),
                          expandedUtf8.constData());
    editor->endUndoAction();

    // Promote parsed stops to absolute byte offsets in the buffer, then
    // sort them as 1, 2, 3, ..., N, then 0 (final caret).
    QList<Stop> stops;
    stops.reserve(parsed.size());
    for (const ParsedStop& p : parsed) {
        Stop s;
        s.number = p.number;
        s.pos    = static_cast<int>(replaceStart) + p.byteStart;
        s.len    = p.byteLen;
        stops.append(s);
    }
    std::sort(stops.begin(), stops.end(), [](const Stop& a, const Stop& b) {
        // 0 is the exit position — always last.
        const int an = (a.number == 0) ? std::numeric_limits<int>::max() : a.number;
        const int bn = (b.number == 0) ? std::numeric_limits<int>::max() : b.number;
        return an < bn;
    });

    if (stops.isEmpty()) {
        editor->setSel(replaceStart + expandedUtf8.size(),
                       replaceStart + expandedUtf8.size());
        return true;
    }

    m_session.editor       = editor;
    m_session.stops        = stops;
    m_session.currentIndex = -1;

    connect(editor, &ScintillaEditBase::modified,
            this, &Snippets::onEditorModified, Qt::UniqueConnection);

    selectStop(0);
    return true;
}

bool Snippets::selectStop(int index)
{
    if (!m_session.editor || index < 0 || index >= m_session.stops.size()) {
        endSession();
        return false;
    }
    auto* sci = m_session.editor.data();
    const Stop& s = m_session.stops[index];

    // Stop 0 is the "final caret" position — collapse selection there and
    // end the session.
    if (s.number == 0) {
        sci->setSel(s.pos, s.pos);
        endSession();
        return false;
    }
    sci->setSel(s.pos, s.pos + s.len);
    m_session.currentIndex = index;
    return true;
}

bool Snippets::hasActiveSession(ScintillaEdit* editor) const {
    return m_session.editor && m_session.editor.data() == editor
           && m_session.currentIndex >= 0;
}

bool Snippets::advanceTabstop(ScintillaEdit* editor) {
    if (!hasActiveSession(editor)) return false;
    return selectStop(m_session.currentIndex + 1);
}

bool Snippets::retreatTabstop(ScintillaEdit* editor) {
    if (!hasActiveSession(editor)) return false;
    if (m_session.currentIndex <= 0) return true;   // consume but stay
    return selectStop(m_session.currentIndex - 1);
}

void Snippets::cancelSession(ScintillaEdit* /*editor*/) {
    endSession();
}

void Snippets::endSession()
{
    if (m_session.editor) {
        disconnect(m_session.editor.data(), &ScintillaEditBase::modified,
                   this, &Snippets::onEditorModified);
    }
    m_session = Session{};
}

void Snippets::onEditorModified(Scintilla::ModificationFlags type,
                                Scintilla::Position position,
                                Scintilla::Position length,
                                Scintilla::Position /*linesAdded*/,
                                const QByteArray& /*text*/,
                                Scintilla::Position /*line*/,
                                Scintilla::FoldLevel /*foldNow*/,
                                Scintilla::FoldLevel /*foldPrev*/)
{
    if (!m_session.editor || m_session.currentIndex < 0) return;

    const auto mask = Scintilla::ModificationFlags::InsertText
                    | Scintilla::ModificationFlags::DeleteText;
    if (static_cast<int>(type & mask) == 0) return;

    const bool insert = (static_cast<int>(type & Scintilla::ModificationFlags::InsertText) != 0);
    const int delta = insert ? static_cast<int>(length) : -static_cast<int>(length);
    const int changeAt = static_cast<int>(position);

    Stop& current = m_session.stops[m_session.currentIndex];

    // Edit inside the active stop: stretch / shrink its length, but keep
    // its starting position the same.
    if (changeAt >= current.pos && changeAt <= current.pos + current.len) {
        current.len = qMax(0, current.len + delta);
    } else if (changeAt < current.pos) {
        // Edit *before* the active stop (rare — usually means user moved
        // out and started typing): nudge our anchor too.
        current.pos += delta;
    }

    // Shift every stop that lives strictly after the change point.
    for (int i = 0; i < m_session.stops.size(); ++i) {
        if (i == m_session.currentIndex) continue;
        Stop& s = m_session.stops[i];
        if (s.pos > changeAt) s.pos += delta;
    }
}
