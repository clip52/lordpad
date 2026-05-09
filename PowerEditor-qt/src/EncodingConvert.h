#pragma once

// =============================================================================
// EncodingConvert
// -----------------------------------------------------------------------------
// Módulo M7 — converte o conteúdo do buffer atual entre codificações comuns
// (UTF-8, UTF-8 com BOM, UTF-16 LE/BE, Latin-1, Windows-1252, Windows-1250).
//
// Importante:
//   - O ScintillaEdit guarda o texto SEMPRE como UTF-8 internamente
//     (SCI_SETCODEPAGE = SC_CP_UTF8 = 65001). Por isso a "conversão" aqui é:
//       1) ler o conteúdo do editor (UTF-8)              → QString
//       2) re-encodar para a codificação alvo            → QByteArray
//       3) re-decodar de volta para UTF-8                → QString
//       4) substituir o buffer (a representação interna continua UTF-8;
//          caracteres não suportados pelo alvo viram '?')
//   - A escolha do encoding é persistida em QSettings
//     ("clip52" / "notepadpp-qt", grupo "FileEncoding") por path.
//     FileIO ainda não consome essa configuração — fica como contrato
//     pra integração futura (salvar de fato no encoding escolhido).
// =============================================================================

#include <QObject>
#include <QPointer>
#include <QString>
#include <QByteArray>

class QMenu;
class QAction;
class QActionGroup;
class QWidget;
class ScintillaEdit;

class EncodingConvert : public QObject
{
    Q_OBJECT
public:
    enum class Encoding {
        Utf8,
        Utf8Bom,
        Utf16LE,
        Utf16BE,
        Latin1,
        Windows1252,
        Windows1250
    };
    Q_ENUM(Encoding)

    explicit EncodingConvert(QObject* parent = nullptr);

    // Cria (ou retorna em cache) o submenu pt-BR "Codificação".
    QMenu* createMenu(QWidget* parent);

    // Vincula o editor ativo + path do arquivo (path pode ser vazio em
    // buffers ainda não salvos; a persistência em QSettings é pulada nesse caso).
    void setActiveEditor(ScintillaEdit* editor, const QString& filePath);

    // Sincroniza o radio do menu com o encoding salvo para o path corrente.
    void syncCheckedFromSettings();

    // Helpers públicos.
    static QString    displayName(Encoding e);     // rótulo pt-BR
    static QByteArray encodingId(Encoding e);      // id do codec / tag interna

    // Look up the encoding choice persisted for `filePath`. Returns Utf8 when
    // there is no entry (also when filePath is empty).
    static Encoding loadFor(const QString& filePath);

    // Persist `e` as the encoding choice for `filePath`. No-op for empty path.
    static void saveFor(const QString& filePath, Encoding e);

    // Encoding ⇒ codec name accepted by QStringEncoder / FileIO::writeFileEncoded.
    static QString codecName(Encoding e);

    // Whether the encoding writes a BOM (UTF-8 with BOM, UTF-16LE/BE).
    static bool wantsBOM(Encoding e);

    // Map a free-form id (output of detectEncoding / encodingId) back to enum.
    // Returns Utf8 for unknown ids.
    static Encoding fromIdString(const QString& id);

public slots:
    // Aplica a conversão ao buffer ativo. Mostra aviso se for lossy.
    void convertTo(Encoding target);

signals:
    void encodingChanged(const QString& filePath, EncodingConvert::Encoding e);

private:
    void buildActions();
    void updateActionsEnabled();
    void persistChoice(Encoding e) const;
    Encoding loadChoice() const;

    QPointer<ScintillaEdit> m_editor;
    QString                 m_filePath;

    QPointer<QMenu>         m_menu;
    QPointer<QActionGroup>  m_group;

    QPointer<QAction>       m_actUtf8;
    QPointer<QAction>       m_actUtf8Bom;
    QPointer<QAction>       m_actUtf16Le;
    QPointer<QAction>       m_actUtf16Be;
    QPointer<QAction>       m_actLatin1;
    QPointer<QAction>       m_actWin1252;
    QPointer<QAction>       m_actWin1250;
};
