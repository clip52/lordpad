#pragma once
//
// ColorMarkers — 5 marcadores de linha coloridos independentes na margem do
// Scintilla (azul, vermelho, verde, amarelo, magenta). Permite que o usuário
// organize visualmente um arquivo: "TODO" em vermelho, "ok" em verde, etc.
//
// Independente do BookmarkManager: usa marker numbers 10..14 (Scintilla pode
// reservar 0..9 para folding/bookmark). SC_MARK_BACKGROUND com alpha baixo
// pinta o fundo da linha inteira, semi-transparente.
//
// Persistência: QSettings("clip52","notepadpp-qt"), grupo "ColorMarkers",
// chave por path absoluto, valor = QStringList no formato "color:line".
//

#include <QObject>
#include <QHash>
#include <QString>
#include <QColor>
#include <QList>

class ScintillaEdit;

enum class MarkColor {
    Blue    = 0,
    Red     = 1,
    Green   = 2,
    Yellow  = 3,
    Magenta = 4
};

class ColorMarkers : public QObject {
    Q_OBJECT
public:
    explicit ColorMarkers(QObject* parent = nullptr);

    // Liga um editor a um filePath. Aplica definições de marker e restaura
    // marcadores persistidos para esse path. Se filePath estiver vazio, só
    // configura os markers (sem persistência).
    void attach(ScintillaEdit* editor, const QString& filePath);

    // Desliga o editor: salva o estado atual dos marcadores no QSettings (se
    // houver path conhecido) e remove o tracking interno.
    void detach(ScintillaEdit* editor);

    // Toggle do marcador da cor `color` na linha do caret.
    void toggle(ScintillaEdit* editor, MarkColor color);

    // Pula o caret pro próximo marcador da cor `color` (com wrap).
    void next(ScintillaEdit* editor, MarkColor color);

    // Remove todos os marcadores da cor `color` no editor.
    void clearAll(ScintillaEdit* editor, MarkColor color);

    // Salva manualmente o estado do editor (chamado em detach e ao salvar
    // arquivo, por ex.). Idempotente.
    void persist(ScintillaEdit* editor);

    // Helpers públicos para a UI (menu/atalhos).
    static QString colorName(MarkColor c);   // pt-BR: "Azul", "Vermelho", etc.
    static QColor  colorOf(MarkColor c);

    // Constantes expostas para conveniência
    static constexpr int kFirstMarker = 10;
    static constexpr int kMarkerCount = 5;

private:
    // Aplica markerDefine/markerSetBack/markerSetAlpha para todos os 5 markers
    // no editor dado. Idempotente — pode ser chamado múltiplas vezes.
    void configureMarkers(ScintillaEdit* editor);

    // Restaura marcadores do QSettings para um editor recém-anexado.
    void restoreFor(ScintillaEdit* editor, const QString& filePath);

    // Salva o estado atual de um editor para um filePath em QSettings.
    void saveFor(ScintillaEdit* editor, const QString& filePath);

    // Converte MarkColor <-> índice de marcador no Scintilla.
    static int markerNumber(MarkColor c) { return kFirstMarker + static_cast<int>(c); }
    static int markerMask(MarkColor c)   { return 1 << markerNumber(c); }

    // Map editor -> filePath. Nada de QPointer para manter cabeçalho leve;
    // o caller é responsável por chamar detach() antes de destruir o editor.
    QHash<ScintillaEdit*, QString> m_paths;
};
