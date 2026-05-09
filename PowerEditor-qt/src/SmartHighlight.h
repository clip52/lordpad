#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>

#include "ScintillaEdit.h"

class QTimer;

// SmartHighlight — destaca todas as ocorrências da palavra sob o
// cursor (ou da seleção atual) em todas as instâncias de
// ScintillaEdit anexadas. Usa o indicador Scintilla número 9
// (logo acima do 8 reservado por UrlHyperlink), desenhado como
// uma caixa amarela translúcida.
//
// O módulo é stand-alone: nenhuma alteração nos arquivos existentes
// é necessária. Basta chamar SmartHighlight::installFor(editor) para
// cada novo ScintillaEdit, e (opcionalmente) SmartHighlight::shared()
// para alternar o estado global / ajustar parâmetros.
//
// As preferências (enabled / match-case / whole-word / cor) são
// persistidas em QSettings("clip52","notepadpp-qt") sob a seção
// "SmartHighlight".
class SmartHighlight : public QObject {
    Q_OBJECT
public:
    // Indicador Scintilla usado para marcar matches. Escolhido
    // imediatamente acima do indicador 8 do UrlHyperlink para evitar
    // colisão com lexers (que costumam usar 0..7).
    static constexpr int kIndicator = 9;

    explicit SmartHighlight(QObject* parent = nullptr);
    ~SmartHighlight() override;

    // Singleton compartilhado — usado por installFor() e pelo
    // MainWindow para alternar o estado a partir do menu.
    static SmartHighlight& shared();

    // Atalho para attach() via singleton.
    static void installFor(ScintillaEdit* editor);

    // Anexa o comportamento a um ScintillaEdit. Idempotente: anexar
    // o mesmo editor duas vezes é no-op. O detach acontece
    // automaticamente quando o editor é destruído.
    void attach(ScintillaEdit* editor);

    // Desanexa o módulo do editor. Indicadores são limpos.
    void detach(ScintillaEdit* editor);

    // Liga/desliga globalmente em todos os editores anexados.
    // Persiste em QSettings ("SmartHighlight/Enabled").
    void setEnabled(bool on);
    bool isEnabled() const { return m_enabled; }

    // Configura case-sensitivity. Persiste em QSettings.
    void setMatchCase(bool on);
    bool matchCase() const { return m_matchCase; }

    // Configura whole-word matching. Persiste em QSettings.
    void setWholeWord(bool on);
    bool wholeWord() const { return m_wholeWord; }

private slots:
    void onEditorDestroyed(QObject* obj);
    void onUpdateUi(Scintilla::Update updated);
    void onRescanTimeout();

private:
    struct EditorEntry {
        QPointer<ScintillaEdit> editor;
        QTimer* debounce = nullptr;
        // Última palavra marcada (em UTF-8) — evita rescans
        // redundantes quando o caret se move dentro do mesmo token.
        QByteArray lastWord;
    };

    void loadSettings();
    void saveBool(const char* key, bool value) const;

    void configureIndicator(ScintillaEdit* editor) const;
    void clearIndicators(ScintillaEdit* editor) const;
    void scheduleRescan(ScintillaEdit* editor);
    void rescan(ScintillaEdit* editor);

    // Extrai a palavra atual (ou seleção) como UTF-8. Retorna
    // QByteArray vazio se não houver palavra elegível para destaque.
    QByteArray currentTarget(ScintillaEdit* editor) const;

    bool m_enabled    = true;
    bool m_matchCase  = true;
    bool m_wholeWord  = true;
    // Cor amarelo translúcido em formato Scintilla 0x00BBGGRR.
    // Default 0x00FFFF (B=0x00, G=0xFF, R=0xFF) -> amarelo.
    sptr_t m_color    = 0x0000FFFF;
    int m_alpha       = 100;   // 0..255 — translucidez da caixa.
    int m_outline     = 150;   // 0..255 — translucidez da borda.

    QHash<ScintillaEdit*, EditorEntry> m_entries;
};
