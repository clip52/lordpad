#include "ColorMarkers.h"

#include "ScintillaEdit.h"

#include <QSettings>
#include <QStringList>
#include <QStringBuilder>

namespace {

// Configurações persistentes
constexpr const char* kOrg     = "clip52";
constexpr const char* kApp     = "notepadpp-qt";
constexpr const char* kGroup   = "ColorMarkers";

// Alpha (0..255) — baixo o suficiente para o texto continuar legível.
constexpr int kMarkerAlpha = 80;

// Converte um QColor para o formato BGR de 24 bits que o Scintilla espera em
// markerSetBack (R no byte baixo, B no alto).
inline sptr_t scintillaBGR(const QColor& c)
{
    return (sptr_t(c.blue()) << 16) | (sptr_t(c.green()) << 8) | sptr_t(c.red());
}

// Lista das 5 cores em ordem (Blue=0..Magenta=4). Cores escolhidas com
// saturação média para combinar com fundos claros e escuros — o alpha 80
// suaviza ainda mais.
const QColor& paletteEntry(int idx)
{
    static const QColor kPalette[] = {
        QColor( 64, 128, 255), // Blue
        QColor(255,  80,  80), // Red
        QColor( 80, 200, 100), // Green
        QColor(240, 210,  60), // Yellow
        QColor(220,  90, 220), // Magenta
    };
    return kPalette[idx];
}

} // namespace

// ---------------------------------------------------------------------------
// Construção
// ---------------------------------------------------------------------------

ColorMarkers::ColorMarkers(QObject* parent)
    : QObject(parent)
{
}

// ---------------------------------------------------------------------------
// API estática (cores e nomes)
// ---------------------------------------------------------------------------

QColor ColorMarkers::colorOf(MarkColor c)
{
    return paletteEntry(static_cast<int>(c));
}

QString ColorMarkers::colorName(MarkColor c)
{
    switch (c) {
    case MarkColor::Blue:    return QStringLiteral("Azul");
    case MarkColor::Red:     return QStringLiteral("Vermelho");
    case MarkColor::Green:   return QStringLiteral("Verde");
    case MarkColor::Yellow:  return QStringLiteral("Amarelo");
    case MarkColor::Magenta: return QStringLiteral("Magenta");
    }
    return QString();
}

// ---------------------------------------------------------------------------
// attach / detach
// ---------------------------------------------------------------------------

void ColorMarkers::attach(ScintillaEdit* editor, const QString& filePath)
{
    if (!editor)
        return;

    configureMarkers(editor);
    m_paths.insert(editor, filePath);

    if (!filePath.isEmpty())
        restoreFor(editor, filePath);
}

void ColorMarkers::detach(ScintillaEdit* editor)
{
    if (!editor)
        return;

    // Salva antes de soltar o editor.
    auto it = m_paths.find(editor);
    if (it != m_paths.end()) {
        if (!it.value().isEmpty())
            saveFor(editor, it.value());
        m_paths.erase(it);
    }
}

void ColorMarkers::persist(ScintillaEdit* editor)
{
    if (!editor)
        return;
    auto it = m_paths.constFind(editor);
    if (it == m_paths.constEnd())
        return;
    if (it.value().isEmpty())
        return;
    saveFor(editor, it.value());
}

// ---------------------------------------------------------------------------
// Operações de marcador
// ---------------------------------------------------------------------------

void ColorMarkers::toggle(ScintillaEdit* editor, MarkColor color)
{
    if (!editor)
        return;

    const int marker = markerNumber(color);
    const int mask   = markerMask(color);

    const sptr_t pos  = editor->currentPos();
    const sptr_t line = editor->lineFromPosition(pos);
    const sptr_t cur  = editor->markerGet(line);

    if (cur & mask)
        editor->markerDelete(line, marker);
    else
        editor->markerAdd(line, marker);

    persist(editor);
}

void ColorMarkers::next(ScintillaEdit* editor, MarkColor color)
{
    if (!editor)
        return;

    const int mask = markerMask(color);

    const sptr_t pos     = editor->currentPos();
    const sptr_t curLine = editor->lineFromPosition(pos);

    // Procura a partir da linha seguinte; se não achar, faz wrap a partir do topo.
    sptr_t target = editor->markerNext(curLine + 1, mask);
    if (target < 0)
        target = editor->markerNext(0, mask);
    if (target < 0)
        return; // sem marcadores dessa cor

    editor->gotoLine(target);
}

void ColorMarkers::clearAll(ScintillaEdit* editor, MarkColor color)
{
    if (!editor)
        return;
    editor->markerDeleteAll(markerNumber(color));
    persist(editor);
}

// ---------------------------------------------------------------------------
// Configuração dos markers no Scintilla
// ---------------------------------------------------------------------------

void ColorMarkers::configureMarkers(ScintillaEdit* editor)
{
    for (int i = 0; i < kMarkerCount; ++i) {
        const int marker = kFirstMarker + i;
        const QColor& c  = paletteEntry(i);

        // SC_MARK_BACKGROUND pinta a linha inteira em vez de desenhar um símbolo
        // na margem — fica mais visível para "marcação semântica" de linhas.
        editor->markerDefine(marker, SC_MARK_BACKGROUND);
        editor->markerSetBack(marker, scintillaBGR(c));
        editor->markerSetAlpha(marker, kMarkerAlpha);
    }
}

// ---------------------------------------------------------------------------
// Persistência (QSettings)
// ---------------------------------------------------------------------------

void ColorMarkers::restoreFor(ScintillaEdit* editor, const QString& filePath)
{
    QSettings s(QString::fromLatin1(kOrg), QString::fromLatin1(kApp));
    s.beginGroup(QString::fromLatin1(kGroup));
    const QStringList raw = s.value(filePath).toStringList();
    s.endGroup();

    if (raw.isEmpty())
        return;

    // Cada entrada é "color:line" — color em [0..4], line 0-based.
    for (const QString& entry : raw) {
        const int sep = entry.indexOf(QLatin1Char(':'));
        if (sep <= 0)
            continue;
        bool okC = false, okL = false;
        const int c = entry.left(sep).toInt(&okC);
        const int l = entry.mid(sep + 1).toInt(&okL);
        if (!okC || !okL)
            continue;
        if (c < 0 || c >= kMarkerCount)
            continue;
        if (l < 0)
            continue;
        // Não verificamos contra lineCount() — o Scintilla aceita markerAdd em
        // qualquer linha existente. Se o arquivo encolheu, o marker simplesmente
        // não aparece (Scintilla descarta linhas inválidas).
        editor->markerAdd(l, kFirstMarker + c);
    }
}

void ColorMarkers::saveFor(ScintillaEdit* editor, const QString& filePath)
{
    QStringList entries;

    for (int c = 0; c < kMarkerCount; ++c) {
        const int marker = kFirstMarker + c;
        const int mask   = 1 << marker;

        sptr_t line = 0;
        while (true) {
            const sptr_t found = editor->markerNext(line, mask);
            if (found < 0)
                break;
            entries.append(QString::number(c) % QLatin1Char(':') % QString::number(static_cast<int>(found)));
            line = found + 1;
        }
    }

    QSettings s(QString::fromLatin1(kOrg), QString::fromLatin1(kApp));
    s.beginGroup(QString::fromLatin1(kGroup));
    if (entries.isEmpty())
        s.remove(filePath); // não polui o registry com chaves vazias
    else
        s.setValue(filePath, entries);
    s.endGroup();
}
