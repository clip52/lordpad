#include "LspFeatures.h"

#include "EditorTab.h"
#include "LspClient.h"
#include "LexerMap.h"
#include "MultiView.h"

#include <ScintillaEdit.h>

#include <QPointer>
#include <QPoint>

namespace {

// Walk a UTF-8 byte buffer counting how many UTF-16 code units it would
// produce, stopping after `byteCount` bytes (or end-of-buffer first). Used
// to map Scintilla's byte caret position into LSP's character index, which
// is defined as offsets in UTF-16 code units within the line.
int utf16UnitsForUtf8Prefix(const char* data, int byteCount)
{
    int units = 0;
    int i = 0;
    while (i < byteCount) {
        const unsigned char c = static_cast<unsigned char>(data[i]);
        int cpLen;
        unsigned int cp;
        if      (c < 0x80)         { cpLen = 1; cp = c; }
        else if ((c & 0xE0) == 0xC0) { cpLen = 2; cp = c & 0x1F; }
        else if ((c & 0xF0) == 0xE0) { cpLen = 3; cp = c & 0x0F; }
        else if ((c & 0xF8) == 0xF0) { cpLen = 4; cp = c & 0x07; }
        else                          { cpLen = 1; cp = 0xFFFD; }   // invalid → replacement

        if (i + cpLen > byteCount) break;
        for (int k = 1; k < cpLen; ++k) {
            cp = (cp << 6) | (static_cast<unsigned char>(data[i + k]) & 0x3F);
        }
        // Code points outside the BMP encode to a UTF-16 surrogate pair = 2 code units.
        units += (cp > 0xFFFF) ? 2 : 1;
        i += cpLen;
    }
    return units;
}

// Inverse: given a UTF-8 line and a UTF-16 character offset within it,
// return the byte offset that corresponds to that character index.
// Clamps to byteCount when the line is shorter than `chars` units.
int utf8BytesForUtf16Prefix(const char* data, int byteCount, int chars)
{
    int units = 0;
    int i = 0;
    while (i < byteCount && units < chars) {
        const unsigned char c = static_cast<unsigned char>(data[i]);
        int cpLen;
        unsigned int cp;
        if      (c < 0x80)         { cpLen = 1; cp = c; }
        else if ((c & 0xE0) == 0xC0) { cpLen = 2; cp = c & 0x1F; }
        else if ((c & 0xF0) == 0xE0) { cpLen = 3; cp = c & 0x0F; }
        else if ((c & 0xF8) == 0xF0) { cpLen = 4; cp = c & 0x07; }
        else                          { cpLen = 1; cp = 0xFFFD; }

        if (i + cpLen > byteCount) break;
        for (int k = 1; k < cpLen; ++k) {
            cp = (cp << 6) | (static_cast<unsigned char>(data[i + k]) & 0x3F);
        }
        const int newUnits = units + ((cp > 0xFFFF) ? 2 : 1);
        if (newUnits > chars) break;   // would overshoot: stop on the codepoint boundary
        units = newUnits;
        i += cpLen;
    }
    return i;
}

// Map LSP CompletionItemKind → short type tag shown after each entry in the
// Scintilla autocomplete popup (e.g. "myFunc?fun"). The numeric kind is what
// Scintilla expects after "?"; we register an icon registry below.
char autoCKindGlyph(int lspKind)
{
    // Coarse buckets: function-like, type-like, value-like, plain.
    switch (lspKind) {
        case 2: case 3: case 4:                return 'f';   // Method/Function/Constructor
        case 5: case 10:                       return 'p';   // Field/Property
        case 6: case 21:                       return 'v';   // Variable/Constant
        case 7: case 8: case 22: case 25:      return 't';   // Class/Interface/Struct/TypeParam
        case 13: case 19: case 20:             return 'e';   // Enum/Folder/EnumMember
        case 14:                               return 'k';   // Keyword
        default:                               return 'i';   // Generic identifier
    }
}

} // namespace

LspFeatures::LspFeatures(LspClient* lsp, MultiView* multi, QObject* parent)
    : QObject(parent), m_lsp(lsp), m_multi(multi)
{
}

void LspFeatures::setActiveTab(EditorTab* tab)
{
    m_activeTab = tab;
    if (tab && tab->editor()) ensureBoundEditor(tab->editor());
}

void LspFeatures::ensureBoundEditor(ScintillaEdit* sci)
{
    if (!sci) return;
    auto& b = m_binds[sci];
    if (b.dwellInstalled) return;

    b.editor = sci;
    b.dwellInstalled = true;

    // 700 ms is the conventional dwell threshold (matches VS Code's hover delay).
    sci->setMouseDwellTime(700);

    connect(sci, &ScintillaEditBase::dwellStart, this,
            [this, sci](int x, int y) {
                if (!m_activeTab || m_activeTab->editor() != sci) return;
                onDwellStart(x, y, /*position*/ -1);
            });
}

void LspFeatures::onDwellStart(int x, int y, int /*positionUnused*/)
{
    if (!m_lsp || !m_activeTab) return;
    auto* sci = m_activeTab->editor();
    if (!sci) return;

    const int pos = static_cast<int>(sci->positionFromPoint(x, y));
    if (pos < 0) return;

    int line = 0, ch = 0;
    if (!sciPositionToLsp(sci, pos, line, ch)) return;

    // Translate to global coordinates so MainWindow can place a QToolTip there.
    const QPoint global = sci->mapToGlobal(QPoint(x, y));
    QPointer<LspFeatures> self(this);
    QPointer<EditorTab> tab(m_activeTab);
    m_lsp->requestHover(m_activeTab->filePath(), line, ch,
        [self, tab, global](const LspHover& h) {
            if (!self || !tab) return;
            if (h.text.isEmpty()) return;
            emit self->hoverReady(h.text, global.x(), global.y());
        });
}

void LspFeatures::requestHoverAtCaret()
{
    if (!m_lsp || !m_activeTab || !m_activeTab->editor()) return;
    auto* sci = m_activeTab->editor();
    const int pos = static_cast<int>(sci->currentPos());
    int line = 0, ch = 0;
    if (!sciPositionToLsp(sci, pos, line, ch)) return;

    const int px = static_cast<int>(sci->pointXFromPosition(pos));
    const int py = static_cast<int>(sci->pointYFromPosition(pos));
    const QPoint global = sci->mapToGlobal(QPoint(px, py));
    QPointer<LspFeatures> self(this);
    m_lsp->requestHover(m_activeTab->filePath(), line, ch,
        [self, global](const LspHover& h) {
            if (!self) return;
            if (h.text.isEmpty()) return;
            emit self->hoverReady(h.text, global.x(), global.y());
        });
}

void LspFeatures::requestDefinitionAtCaret()
{
    if (!m_lsp || !m_activeTab || !m_activeTab->editor()) return;
    auto* sci = m_activeTab->editor();
    const int pos = static_cast<int>(sci->currentPos());
    int line = 0, ch = 0;
    if (!sciPositionToLsp(sci, pos, line, ch)) return;

    QPointer<LspFeatures> self(this);
    m_lsp->requestDefinition(m_activeTab->filePath(), line, ch,
        [self](const QList<LspLocation>& locs) {
            if (!self || locs.isEmpty()) return;
            // Take the first target — multi-target nav is a future feature.
            const LspLocation& l = locs.first();
            emit self->definitionResolved(l.filePath, l.line, l.column);
        });
}

void LspFeatures::requestCompletionAtCaret()
{
    if (!m_lsp || !m_activeTab || !m_activeTab->editor()) return;
    auto* sci = m_activeTab->editor();
    const int caret = static_cast<int>(sci->currentPos());

    // Compute the LSP position for the caret.
    int line = 0, ch = 0;
    if (!sciPositionToLsp(sci, caret, line, ch)) return;

    // Compute lenEntered for autoCShow: bytes between the start of the current
    // word and the caret. Scintilla's "word characters" already match what we
    // want for completion prefix matching.
    const int wordStart = static_cast<int>(sci->wordStartPosition(caret, true));
    const int lenEntered = caret - wordStart;

    QPointer<LspFeatures> self(this);
    QPointer<ScintillaEdit> editor(sci);
    m_lsp->requestCompletion(m_activeTab->filePath(), line, ch,
        [self, editor, lenEntered](const QList<LspCompletionItem>& items) {
            if (!self || !editor || items.isEmpty()) return;

            // Build a '\n'-separated list because labels frequently contain spaces.
            // Scintilla supports per-item "type" tags after a '?' character —
            // we use a single-letter glyph per kind, see autoCKindGlyph().
            QByteArray list;
            list.reserve(items.size() * 24);
            for (int i = 0; i < items.size(); ++i) {
                if (i > 0) list += '\n';
                list += items[i].label.toUtf8();
                list += '?';
                list += autoCKindGlyph(items[i].kind);
            }
            editor->autoCSetSeparator('\n');
            editor->autoCShow(lenEntered, list.constData());
        });
}

bool LspFeatures::sciPositionToLsp(ScintillaEdit* sci, int byteOffset,
                                   int& outLine, int& outCharacter) const
{
    if (!sci) return false;
    const int line = static_cast<int>(sci->lineFromPosition(byteOffset));
    const int lineStart = static_cast<int>(sci->positionFromLine(line));
    const int bytesIntoLine = byteOffset - lineStart;

    // sci->getLine returns the line including the line terminator; that's fine
    // for our purposes since we only count up to bytesIntoLine.
    QByteArray lineBytes = sci->getLine(line);
    const int safeBytes = qMin(bytesIntoLine, static_cast<int>(lineBytes.size()));

    outLine      = line;
    outCharacter = utf16UnitsForUtf8Prefix(lineBytes.constData(), safeBytes);
    return true;
}

int LspFeatures::lspPositionToSci(ScintillaEdit* sci, int line, int character) const
{
    if (!sci) return 0;
    const int lineStart = static_cast<int>(sci->positionFromLine(line));
    QByteArray lineBytes = sci->getLine(line);
    const int byteOffsetIntoLine = utf8BytesForUtf16Prefix(
        lineBytes.constData(), lineBytes.size(), character);
    return lineStart + byteOffsetIntoLine;
}

// ---------------------------------------------------------------------------
// M9 request actions
// ---------------------------------------------------------------------------
void LspFeatures::requestSignatureHelpAtCaret()
{
    if (!m_lsp || !m_activeTab || !m_activeTab->editor()) return;
    auto* sci = m_activeTab->editor();
    const int pos = static_cast<int>(sci->currentPos());
    int line = 0, ch = 0;
    if (!sciPositionToLsp(sci, pos, line, ch)) return;

    const int px = static_cast<int>(sci->pointXFromPosition(pos));
    const int py = static_cast<int>(sci->pointYFromPosition(pos));
    const QPoint global = sci->mapToGlobal(QPoint(px, py));

    QPointer<LspFeatures> self(this);
    m_lsp->requestSignatureHelp(m_activeTab->filePath(), line, ch,
        [self, global](const LspSignatureHelp& h) {
            if (!self) return;
            if (h.signatures.isEmpty()) return;
            // Render: highlight active signature with a leading "▶ ", others plain.
            QStringList lines;
            for (int i = 0; i < h.signatures.size(); ++i) {
                lines << (i == h.activeSignature ? QStringLiteral("▶ %1").arg(h.signatures[i])
                                                  : QStringLiteral("  %1").arg(h.signatures[i]));
            }
            emit self->signatureHelpReady(lines.join('\n'), global.x(), global.y());
        });
}

void LspFeatures::requestReferencesAtCaret(bool includeDeclaration)
{
    if (!m_lsp || !m_activeTab || !m_activeTab->editor()) return;
    auto* sci = m_activeTab->editor();
    const int pos = static_cast<int>(sci->currentPos());
    int line = 0, ch = 0;
    if (!sciPositionToLsp(sci, pos, line, ch)) return;

    QPointer<LspFeatures> self(this);
    m_lsp->requestReferences(m_activeTab->filePath(), line, ch, includeDeclaration,
        [self](const QList<LspLocation>& locs) {
            if (!self) return;
            emit self->referencesReady(locs);
        });
}

void LspFeatures::requestDocumentSymbolsCurrent()
{
    if (!m_lsp || !m_activeTab) return;
    QPointer<LspFeatures> self(this);
    m_lsp->requestDocumentSymbols(m_activeTab->filePath(),
        [self](const QList<LspSymbol>& syms) {
            if (!self) return;
            emit self->documentSymbolsReady(syms);
        });
}

void LspFeatures::requestWorkspaceSymbolsForQuery(const QString& query)
{
    if (!m_lsp) return;
    QPointer<LspFeatures> self(this);
    m_lsp->requestWorkspaceSymbols(query,
        [self](const QList<LspSymbol>& syms) {
            if (!self) return;
            emit self->workspaceSymbolsReady(syms);
        });
}

void LspFeatures::requestSemanticTokensCurrent()
{
    if (!m_lsp || !m_activeTab) return;
    QPointer<LspFeatures> self(this);
    const QString fp = m_activeTab->filePath();
    m_lsp->requestSemanticTokens(fp,
        [self, fp](const QList<LspSemanticToken>& tokens) {
            if (!self) return;
            emit self->semanticTokensReady(fp, tokens);
        });
}

void LspFeatures::requestOutlineSymbolsCurrent()
{
    if (!m_lsp || !m_activeTab) return;
    QPointer<LspFeatures> self(this);
    const QString fp = m_activeTab->filePath();
    m_lsp->requestDocumentSymbols(fp,
        [self, fp](const QList<LspSymbol>& syms) {
            if (!self) return;
            emit self->outlineSymbolsReady(fp, syms);
        });
}

void LspFeatures::requestRenameAtCaret(const QString& newName)
{
    if (!m_lsp || !m_activeTab || !m_activeTab->editor() || newName.isEmpty()) return;
    auto* sci = m_activeTab->editor();
    const int pos = static_cast<int>(sci->currentPos());
    int line = 0, ch = 0;
    if (!sciPositionToLsp(sci, pos, line, ch)) return;

    QPointer<LspFeatures> self(this);
    const QString captured = newName;
    m_lsp->requestRename(m_activeTab->filePath(), line, ch, newName,
        [self, captured](const QList<LspTextEdit>& edits) {
            if (!self) return;
            emit self->renameEditsReady(edits, captured);
        });
}

void LspFeatures::requestCodeActionsAtCaret()
{
    if (!m_lsp || !m_activeTab || !m_activeTab->editor()) return;
    auto* sci = m_activeTab->editor();
    const int pos = static_cast<int>(sci->currentPos());
    const int line = static_cast<int>(sci->lineFromPosition(pos));
    QPointer<LspFeatures> self(this);
    m_lsp->requestCodeActions(m_activeTab->filePath(), line,
        [self](const QList<LspCodeAction>& actions) {
            if (!self) return;
            emit self->codeActionsReady(actions);
        });
}

void LspFeatures::requestInlayHintsForVisibleRange()
{
    if (!m_lsp || !m_activeTab || !m_activeTab->editor()) return;
    auto* sci = m_activeTab->editor();
    // Visible range: SCI_GETFIRSTVISIBLELINE → docLineFromVisible → +linesOnScreen.
    const int firstVis = static_cast<int>(sci->firstVisibleLine());
    const int firstDoc = static_cast<int>(sci->docLineFromVisible(firstVis));
    const int onScreen = static_cast<int>(sci->linesOnScreen());
    const int lastDoc  = qMin(firstDoc + onScreen + 1,
                              static_cast<int>(sci->lineCount()));

    QPointer<LspFeatures> self(this);
    const QString fp = m_activeTab->filePath();
    m_lsp->requestInlayHints(fp, firstDoc, lastDoc,
        [self, fp](const QList<LspInlayHint>& hints) {
            if (!self) return;
            emit self->inlayHintsReady(fp, hints);
        });
}
