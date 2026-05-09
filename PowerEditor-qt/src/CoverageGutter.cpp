#include "CoverageGutter.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>

#include <ScintillaEdit.h>

namespace {
// Margin number reserved for coverage. Existing margins:
//   0 line numbers
//   1 bookmarks
//   2 fold
//   3 (line state markers in some setups)
//   4 GitDiffGutter
// 5 is free.
constexpr int kMargin   = 5;
constexpr int kHitMark  = 16;   // marker number for "hit"   (32 markers total)
constexpr int kMissMark = 17;   // marker number for "miss"
}

CoverageGutter::CoverageGutter(QObject* parent) : QObject(parent) {}

void CoverageGutter::setEnabled(bool on)
{
    if (m_enabled == on) return;
    m_enabled = on;
    for (auto it = m_attached.begin(); it != m_attached.end(); ++it) {
        if (m_enabled) applyTo(it.key(), it.value());
        else           clearMargin(it.key());
    }
}

void CoverageGutter::attach(ScintillaEdit* editor, const QString& filePath)
{
    if (!editor) return;
    m_attached.insert(editor, filePath);
    configureEditor(editor);
    if (m_enabled) applyTo(editor, filePath);
}

void CoverageGutter::detach(ScintillaEdit* editor)
{
    if (!editor) return;
    clearMargin(editor);
    m_attached.remove(editor);
}

void CoverageGutter::configureEditor(ScintillaEdit* editor)
{
    editor->setMarginWidthN(kMargin, 6);
    editor->setMarginTypeN(kMargin, 2 /*SC_MARGIN_SYMBOL*/);
    editor->setMarginMaskN(kMargin, (1 << kHitMark) | (1 << kMissMark));
    editor->markerDefine(kHitMark,  3 /*SC_MARK_PLUS*/);  // We override to FULLRECT below.
    editor->markerDefine(kHitMark,  9 /*SC_MARK_FULLRECT*/);
    editor->markerDefine(kMissMark, 9 /*SC_MARK_FULLRECT*/);
    editor->markerSetBack(kHitMark,  0x004FAE4F);   // green
    editor->markerSetBack(kMissMark, 0x003E3EE5);   // red (Scintilla 0x00BBGGRR)
}

void CoverageGutter::clearMargin(ScintillaEdit* editor)
{
    if (!editor) return;
    editor->markerDeleteAll(kHitMark);
    editor->markerDeleteAll(kMissMark);
}

void CoverageGutter::applyTo(ScintillaEdit* editor, const QString& filePath)
{
    if (!editor) return;
    clearMargin(editor);

    // Try direct path first; fall back to basename match (lcov stores absolute,
    // user might have opened the file from a different relative root).
    Coverage cov;
    auto it = m_coverage.find(filePath);
    if (it != m_coverage.end()) {
        cov = it.value();
    } else {
        const QString base = QFileInfo(filePath).fileName();
        for (auto j = m_coverage.constBegin(); j != m_coverage.constEnd(); ++j) {
            if (QFileInfo(j.key()).fileName() == base) { cov = j.value(); break; }
        }
    }

    for (int line : cov.hits)   editor->markerAdd(line, kHitMark);
    for (int line : cov.misses) editor->markerAdd(line, kMissMark);
}

bool CoverageGutter::loadLcovTracefile(const QString& path, QString* outErr)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (outErr) *outErr = tr("Não consegui abrir %1").arg(path);
        return false;
    }
    QTextStream in(&f);
    QString currentFile;
    Coverage cur;
    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.startsWith(QStringLiteral("SF:"))) {
            if (!currentFile.isEmpty()) m_coverage.insert(currentFile, cur);
            currentFile = line.mid(3);
            cur = Coverage{};
        } else if (line.startsWith(QStringLiteral("DA:"))) {
            // DA:<line>,<count>
            const QStringList parts = line.mid(3).split(',');
            if (parts.size() < 2) continue;
            const int sourceLine = parts[0].toInt() - 1;   // 0-based
            const int hits       = parts[1].toInt();
            if (hits > 0) cur.hits.insert(sourceLine);
            else          cur.misses.insert(sourceLine);
        } else if (line == QStringLiteral("end_of_record")) {
            if (!currentFile.isEmpty()) m_coverage.insert(currentFile, cur);
            currentFile.clear();
            cur = Coverage{};
        }
    }
    if (!currentFile.isEmpty()) m_coverage.insert(currentFile, cur);
    f.close();

    // Re-apply to currently attached editors so we light up immediately.
    for (auto it = m_attached.begin(); it != m_attached.end(); ++it) {
        if (m_enabled) applyTo(it.key(), it.value());
    }
    return true;
}

bool CoverageGutter::loadGcovFile(const QString& gcovPath, QString* outErr)
{
    QFile f(gcovPath);
    if (!f.open(QIODevice::ReadOnly)) {
        if (outErr) *outErr = tr("Não consegui abrir %1").arg(gcovPath);
        return false;
    }
    // .gcov line shape:  "<count>:<lineno>:<source>"
    //   "    -:    1:int main(...)"     unexecuted/non-instrumented
    //   "    1:    5:    return 0;"     hit
    //   "#####:   10:    return 1;"     miss
    QString sourceFile;
    Coverage cov;
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QString line = in.readLine();
        // First few lines have form "        -:    0:Source:foo.cpp"
        if (line.contains(QStringLiteral("0:Source:"))) {
            const int idx = line.indexOf(QStringLiteral("Source:"));
            if (idx >= 0) sourceFile = line.mid(idx + 7).trimmed();
            continue;
        }
        const int firstColon  = line.indexOf(':');
        if (firstColon <= 0) continue;
        const int secondColon = line.indexOf(':', firstColon + 1);
        if (secondColon <= firstColon) continue;
        const QString hitsStr   = line.left(firstColon).trimmed();
        const QString lineNoStr = line.mid(firstColon + 1, secondColon - firstColon - 1).trimmed();
        bool ok = false;
        const int sourceLine = lineNoStr.toInt(&ok) - 1;
        if (!ok || sourceLine < 0) continue;

        if (hitsStr == QStringLiteral("-") || hitsStr.isEmpty()) continue;
        if (hitsStr == QStringLiteral("#####")) cov.misses.insert(sourceLine);
        else                                    cov.hits.insert(sourceLine);
    }
    f.close();
    if (sourceFile.isEmpty()) {
        if (outErr) *outErr = tr("Não consegui inferir o arquivo-fonte do .gcov");
        return false;
    }
    m_coverage.insert(sourceFile, cov);
    for (auto it = m_attached.begin(); it != m_attached.end(); ++it) {
        if (m_enabled) applyTo(it.key(), it.value());
    }
    return true;
}
