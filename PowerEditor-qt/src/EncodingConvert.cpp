#include "EncodingConvert.h"

#include <QAction>
#include <QActionGroup>
#include <QByteArray>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QSignalBlocker>
#include <QString>
#include <QStringConverter>
#include <QStringDecoder>
#include <QStringEncoder>
#include <QWidget>

#include "ScintillaEdit.h"

// -----------------------------------------------------------------------------
// Constantes
// -----------------------------------------------------------------------------
namespace {

// QSettings org/app — alinhado ao restante do projeto.
constexpr const char* kOrg   = "clip52";
constexpr const char* kApp   = "notepadpp-qt";
constexpr const char* kGroup = "FileEncoding";

// Converte o id textual gravado em QSettings de volta pra enum.
EncodingConvert::Encoding fromId(const QByteArray& id)
{
    if (id == "UTF-8")        return EncodingConvert::Encoding::Utf8;
    if (id == "UTF-8-BOM")    return EncodingConvert::Encoding::Utf8Bom;
    if (id == "UTF-16LE")     return EncodingConvert::Encoding::Utf16LE;
    if (id == "UTF-16BE")     return EncodingConvert::Encoding::Utf16BE;
    if (id == "ISO-8859-1")   return EncodingConvert::Encoding::Latin1;
    if (id == "Windows-1252") return EncodingConvert::Encoding::Windows1252;
    if (id == "Windows-1250") return EncodingConvert::Encoding::Windows1250;
    return EncodingConvert::Encoding::Utf8;
}

// Mapeia nosso enum para o QStringConverter::Encoding suportado pelo Qt6.
// Para Latin-1/Win-1252/Win-1250 caímos no fallback Latin1, mas tratamos
// Windows-1252 e Windows-1250 manualmente (tabelas) abaixo.
QStringConverter::Encoding qtEncodingFor(EncodingConvert::Encoding e)
{
    switch (e) {
        case EncodingConvert::Encoding::Utf8:        return QStringConverter::Utf8;
        case EncodingConvert::Encoding::Utf8Bom:     return QStringConverter::Utf8;
        case EncodingConvert::Encoding::Utf16LE:     return QStringConverter::Utf16LE;
        case EncodingConvert::Encoding::Utf16BE:     return QStringConverter::Utf16BE;
        case EncodingConvert::Encoding::Latin1:      return QStringConverter::Latin1;
        case EncodingConvert::Encoding::Windows1252: return QStringConverter::Latin1;
        case EncodingConvert::Encoding::Windows1250: return QStringConverter::Latin1;
    }
    return QStringConverter::Utf8;
}

// ---- Tabelas para Windows-1252 e Windows-1250 -------------------------------
// Apenas a faixa 0x80–0x9F difere de Latin-1; os outros bytes coincidem.
// Valores 0xFFFD = sem mapeamento (caractere de substituição Unicode).
constexpr char16_t kCp1252_80_9F[32] = {
    0x20AC, 0xFFFD, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
    0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0xFFFD, 0x017D, 0xFFFD,
    0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0xFFFD, 0x017E, 0x0178
};

// Windows-1250 difere de Latin-1 em quase toda a faixa alta (Centro-Europeu).
// Entradas a partir de 0xA0; índice 0 = byte 0xA0.
constexpr char16_t kCp1250_A0_FF[96] = {
    0x00A0, 0x02C7, 0x02D8, 0x0141, 0x00A4, 0x0104, 0x00A6, 0x00A7,
    0x00A8, 0x00A9, 0x015E, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x017B,
    0x00B0, 0x00B1, 0x02DB, 0x0142, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
    0x00B8, 0x0105, 0x015F, 0x00BB, 0x013D, 0x02DD, 0x013E, 0x017C,
    0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7,
    0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E,
    0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7,
    0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF,
    0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7,
    0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F,
    0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7,
    0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9
};
constexpr char16_t kCp1250_80_9F[32] = {
    0x20AC, 0xFFFD, 0x201A, 0xFFFD, 0x201E, 0x2026, 0x2020, 0x2021,
    0xFFFD, 0x2030, 0x0160, 0x2039, 0x015A, 0x0164, 0x017D, 0x0179,
    0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0xFFFD, 0x2122, 0x0161, 0x203A, 0x015B, 0x0165, 0x017E, 0x017A
};

// Encoda QString → CP-1252 (substitui ausentes por '?'). Retorna pair{bytes,lossy}.
QPair<QByteArray, bool> encodeCp1252(const QString& s)
{
    QByteArray out;
    out.reserve(s.size());
    bool lossy = false;
    for (QChar qc : s) {
        const char16_t cp = qc.unicode();
        if (cp < 0x80 || (cp >= 0xA0 && cp <= 0xFF)) {
            out.append(static_cast<char>(cp));
            continue;
        }
        // Procura na tabela 0x80–0x9F.
        bool mapped = false;
        for (int i = 0; i < 32; ++i) {
            if (kCp1252_80_9F[i] == cp) {
                out.append(static_cast<char>(0x80 + i));
                mapped = true;
                break;
            }
        }
        if (!mapped) {
            out.append('?');
            lossy = true;
        }
    }
    return {out, lossy};
}

QString decodeCp1252(const QByteArray& b)
{
    QString out;
    out.reserve(b.size());
    for (unsigned char c : b) {
        if (c < 0x80 || c >= 0xA0) {
            out.append(QChar(c));
        } else {
            const char16_t u = kCp1252_80_9F[c - 0x80];
            out.append(u == 0xFFFD ? QChar(QChar::ReplacementCharacter) : QChar(u));
        }
    }
    return out;
}

QPair<QByteArray, bool> encodeCp1250(const QString& s)
{
    QByteArray out;
    out.reserve(s.size());
    bool lossy = false;
    for (QChar qc : s) {
        const char16_t cp = qc.unicode();
        if (cp < 0x80) { out.append(static_cast<char>(cp)); continue; }
        bool mapped = false;
        for (int i = 0; i < 32 && !mapped; ++i) {
            if (kCp1250_80_9F[i] == cp) {
                out.append(static_cast<char>(0x80 + i));
                mapped = true;
            }
        }
        for (int i = 0; i < 96 && !mapped; ++i) {
            if (kCp1250_A0_FF[i] == cp) {
                out.append(static_cast<char>(0xA0 + i));
                mapped = true;
            }
        }
        if (!mapped) { out.append('?'); lossy = true; }
    }
    return {out, lossy};
}

QString decodeCp1250(const QByteArray& b)
{
    QString out;
    out.reserve(b.size());
    for (unsigned char c : b) {
        if (c < 0x80) {
            out.append(QChar(c));
        } else if (c < 0xA0) {
            const char16_t u = kCp1250_80_9F[c - 0x80];
            out.append(u == 0xFFFD ? QChar(QChar::ReplacementCharacter) : QChar(u));
        } else {
            const char16_t u = kCp1250_A0_FF[c - 0xA0];
            out.append(u == 0xFFFD ? QChar(QChar::ReplacementCharacter) : QChar(u));
        }
    }
    return out;
}

} // namespace

// -----------------------------------------------------------------------------
// API estática
// -----------------------------------------------------------------------------
QString EncodingConvert::displayName(Encoding e)
{
    switch (e) {
        case Encoding::Utf8:        return tr("UTF-8");
        case Encoding::Utf8Bom:     return tr("UTF-8 com BOM");
        case Encoding::Utf16LE:     return tr("UTF-16 LE");
        case Encoding::Utf16BE:     return tr("UTF-16 BE");
        case Encoding::Latin1:      return tr("Latin-1 (ISO-8859-1)");
        case Encoding::Windows1252: return tr("Windows-1252 (Ocidental)");
        case Encoding::Windows1250: return tr("Windows-1250 (Centro Europeu)");
    }
    return tr("UTF-8");
}

QByteArray EncodingConvert::encodingId(Encoding e)
{
    switch (e) {
        case Encoding::Utf8:        return "UTF-8";
        case Encoding::Utf8Bom:     return "UTF-8-BOM";
        case Encoding::Utf16LE:     return "UTF-16LE";
        case Encoding::Utf16BE:     return "UTF-16BE";
        case Encoding::Latin1:      return "ISO-8859-1";
        case Encoding::Windows1252: return "Windows-1252";
        case Encoding::Windows1250: return "Windows-1250";
    }
    return "UTF-8";
}

EncodingConvert::Encoding EncodingConvert::fromIdString(const QString& id)
{
    return ::fromId(id.toUtf8());
}

QString EncodingConvert::codecName(Encoding e)
{
    // Names accepted by QStringEncoder / FileIO::writeFileEncoded.
    switch (e) {
        case Encoding::Utf8:        return QStringLiteral("UTF-8");
        case Encoding::Utf8Bom:     return QStringLiteral("UTF-8");
        case Encoding::Utf16LE:     return QStringLiteral("UTF-16LE");
        case Encoding::Utf16BE:     return QStringLiteral("UTF-16BE");
        case Encoding::Latin1:      return QStringLiteral("ISO-8859-1");
        case Encoding::Windows1252: return QStringLiteral("windows-1252");
        case Encoding::Windows1250: return QStringLiteral("windows-1250");
    }
    return QStringLiteral("UTF-8");
}

bool EncodingConvert::wantsBOM(Encoding e)
{
    return e == Encoding::Utf8Bom
        || e == Encoding::Utf16LE
        || e == Encoding::Utf16BE;
}

EncodingConvert::Encoding EncodingConvert::loadFor(const QString& filePath)
{
    if (filePath.isEmpty()) return Encoding::Utf8;
    QSettings s(kOrg, kApp);
    s.beginGroup(kGroup);
    const QByteArray id = s.value(filePath, "UTF-8").toString().toUtf8();
    s.endGroup();
    return ::fromId(id);
}

void EncodingConvert::saveFor(const QString& filePath, Encoding e)
{
    if (filePath.isEmpty()) return;
    QSettings s(kOrg, kApp);
    s.beginGroup(kGroup);
    s.setValue(filePath, QString::fromUtf8(encodingId(e)));
    s.endGroup();
}

// -----------------------------------------------------------------------------
// Construção / menu
// -----------------------------------------------------------------------------
EncodingConvert::EncodingConvert(QObject* parent)
    : QObject(parent)
{
}

QMenu* EncodingConvert::createMenu(QWidget* parent)
{
    if (m_menu)
        return m_menu;

    m_menu  = new QMenu(tr("&Codificação"), parent);
    m_group = new QActionGroup(this);
    m_group->setExclusive(true);

    // Returns the new QAction so callers can store it into a QPointer member
    // (a QPointer can't bind a QAction*& reference — only an implicit T* read).
    auto addRadio = [this](Encoding e) -> QAction* {
        auto* a = m_menu->addAction(displayName(e));
        a->setCheckable(true);
        a->setData(static_cast<int>(e));
        m_group->addAction(a);
        connect(a, &QAction::triggered, this, [this, e]() { convertTo(e); });
        return a;
    };

    m_actUtf8    = addRadio(Encoding::Utf8);
    m_actUtf8Bom = addRadio(Encoding::Utf8Bom);
    m_menu->addSeparator();
    m_actUtf16Le = addRadio(Encoding::Utf16LE);
    m_actUtf16Be = addRadio(Encoding::Utf16BE);
    m_menu->addSeparator();
    m_actLatin1  = addRadio(Encoding::Latin1);
    m_actWin1252 = addRadio(Encoding::Windows1252);
    m_actWin1250 = addRadio(Encoding::Windows1250);

    updateActionsEnabled();
    return m_menu;
}

void EncodingConvert::setActiveEditor(ScintillaEdit* editor, const QString& filePath)
{
    m_editor   = editor;
    m_filePath = filePath;
    updateActionsEnabled();
    syncCheckedFromSettings();
}

void EncodingConvert::updateActionsEnabled()
{
    const bool enabled = !m_editor.isNull();
    if (m_menu) m_menu->setEnabled(enabled);
    for (QAction* a : { m_actUtf8.data(), m_actUtf8Bom.data(),
                        m_actUtf16Le.data(), m_actUtf16Be.data(),
                        m_actLatin1.data(), m_actWin1252.data(),
                        m_actWin1250.data() }) {
        if (a) a->setEnabled(enabled);
    }
}

void EncodingConvert::syncCheckedFromSettings()
{
    if (!m_group) return;
    const Encoding cur = loadChoice();

    QAction* target = nullptr;
    switch (cur) {
        case Encoding::Utf8:        target = m_actUtf8;    break;
        case Encoding::Utf8Bom:     target = m_actUtf8Bom; break;
        case Encoding::Utf16LE:     target = m_actUtf16Le; break;
        case Encoding::Utf16BE:     target = m_actUtf16Be; break;
        case Encoding::Latin1:      target = m_actLatin1;  break;
        case Encoding::Windows1252: target = m_actWin1252; break;
        case Encoding::Windows1250: target = m_actWin1250; break;
    }
    if (target) {
        QSignalBlocker b(m_group);
        target->setChecked(true);
    }
}

// -----------------------------------------------------------------------------
// QSettings
// -----------------------------------------------------------------------------
void EncodingConvert::persistChoice(Encoding e) const
{
    if (m_filePath.isEmpty())
        return;
    QSettings s(kOrg, kApp);
    s.beginGroup(kGroup);
    s.setValue(m_filePath, QString::fromUtf8(encodingId(e)));
    s.endGroup();
}

EncodingConvert::Encoding EncodingConvert::loadChoice() const
{
    if (m_filePath.isEmpty())
        return Encoding::Utf8;
    QSettings s(kOrg, kApp);
    s.beginGroup(kGroup);
    const QByteArray id = s.value(m_filePath, "UTF-8").toString().toUtf8();
    s.endGroup();
    return fromId(id);
}

// -----------------------------------------------------------------------------
// Conversão principal
// -----------------------------------------------------------------------------
void EncodingConvert::convertTo(Encoding target)
{
    if (!m_editor)
        return;

    // 1) lê o conteúdo atual (Scintilla devolve UTF-8).
    const sptr_t len = m_editor->textLength();
    const QByteArray utf8 = m_editor->getText(len + 1);  // +1 reserva NUL
    const QString text = QString::fromUtf8(utf8);

    // 2) re-encoda pra codificação alvo + detecta perdas (lossy).
    QByteArray encoded;
    bool lossy = false;

    switch (target) {
        case Encoding::Utf8: {
            QStringEncoder enc(QStringConverter::Utf8);
            encoded = enc.encode(text);
            break;
        }
        case Encoding::Utf8Bom: {
            QStringEncoder enc(QStringConverter::Utf8);
            encoded = enc.encode(text);
            // O BOM é apenas marcador para a etapa de salvar (FileIO).
            // Para fins de "conteúdo do buffer", o texto interno permanece igual.
            break;
        }
        case Encoding::Utf16LE: {
            QStringEncoder enc(QStringConverter::Utf16LE);
            encoded = enc.encode(text);
            break;
        }
        case Encoding::Utf16BE: {
            QStringEncoder enc(QStringConverter::Utf16BE);
            encoded = enc.encode(text);
            break;
        }
        case Encoding::Latin1: {
            // Latin-1 só representa U+0000..U+00FF — qualquer coisa fora é '?'.
            encoded.reserve(text.size());
            for (QChar qc : text) {
                if (qc.unicode() <= 0xFF) {
                    encoded.append(static_cast<char>(qc.unicode()));
                } else {
                    encoded.append('?');
                    lossy = true;
                }
            }
            break;
        }
        case Encoding::Windows1252: {
            auto pair = encodeCp1252(text);
            encoded = pair.first;
            lossy   = pair.second;
            break;
        }
        case Encoding::Windows1250: {
            auto pair = encodeCp1250(text);
            encoded = pair.first;
            lossy   = pair.second;
            break;
        }
    }

    // Para os codecs Qt, detectamos lossy comparando tamanho/round-trip abaixo.

    // 3) decoda de volta pra UTF-8 (texto que vai voltar pro Scintilla).
    QString roundTripped;
    switch (target) {
        case Encoding::Utf8:
        case Encoding::Utf8Bom: {
            QStringDecoder dec(QStringConverter::Utf8);
            roundTripped = dec.decode(encoded);
            break;
        }
        case Encoding::Utf16LE: {
            QStringDecoder dec(QStringConverter::Utf16LE);
            roundTripped = dec.decode(encoded);
            break;
        }
        case Encoding::Utf16BE: {
            QStringDecoder dec(QStringConverter::Utf16BE);
            roundTripped = dec.decode(encoded);
            break;
        }
        case Encoding::Latin1: {
            roundTripped.reserve(encoded.size());
            for (unsigned char c : encoded) roundTripped.append(QChar(c));
            break;
        }
        case Encoding::Windows1252:
            roundTripped = decodeCp1252(encoded);
            break;
        case Encoding::Windows1250:
            roundTripped = decodeCp1250(encoded);
            break;
    }

    // Detecta lossy nos encodings Qt comparando com o texto original.
    if (!lossy && roundTripped != text)
        lossy = true;

    // 4) confirma com o usuário se a conversão for lossy.
    if (lossy) {
        const auto ans = QMessageBox::warning(
            qobject_cast<QWidget*>(parent()),
            tr("Conversão de codificação"),
            tr("A conversão para %1 é lossy: alguns caracteres não podem ser "
               "representados e serão substituídos por '?'.\n\nDeseja continuar?")
                .arg(displayName(target)),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (ans != QMessageBox::Yes)
            return;
    }

    // 5) substitui o texto do editor + flag de modificação.
    const QByteArray newUtf8 = roundTripped.toUtf8();
    m_editor->beginUndoAction();
    m_editor->setText(newUtf8.constData());
    m_editor->endUndoAction();
    // setText já marca "dirty"; reforçamos limpando savePoint só se necessário
    // (nada a fazer aqui — Scintilla aciona SCN_MODIFIED naturalmente).

    // 6) persiste a escolha (FileIO pode ler isso no futuro).
    persistChoice(target);

    // 7) atualiza estado visual do menu e notifica.
    syncCheckedFromSettings();
    emit encodingChanged(m_filePath, target);
}
