#pragma once
//
// GitDiffGutter
// -------------
// Exibe marcadores de linha (added / modified / removed) na margem 4 do
// Scintilla com base na saída de `git diff --no-color --unified=0 HEAD --
// <relative-path>`. Funciona de forma assíncrona via QProcess e faz
// auto-refresh em modificações do buffer (debounce 1000 ms).
//
// API pública:
//   - attach(editor, filePath): registra o editor, configura a margin e
//     dispara um diff inicial.
//   - detach(editor): remove markers, fecha conexões e cancela queries.
//   - refresh(editor, filePath): re-roda `git diff` sem esperar o debounce
//     (uso típico: após save).
//   - setEnabled(on): persiste em "GitDiffGutter/Enabled". Quando off,
//     limpa markers e ignora refreshes.
//   - isEnabled(): leitura do estado persistido.
//
// Notas de design:
//   - Coalescência: se já há query em voo pra mesmo path, novas chamadas
//     são ignoradas até a primeira terminar.
//   - Margin dedicada (4) pra não conflitar com bookmarks (margin 1) nem
//     com line-number/fold das outras.
//   - Cores fixas: verde (#2ea043 / added), laranja (#d29922 / modified),
//     vermelho (#f85149 / removed) — alinhadas com GitStatusService.
//

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QString>

class ScintillaEdit;
class QProcess;
class QTimer;

class GitDiffGutter : public QObject {
    Q_OBJECT
public:
    explicit GitDiffGutter(QObject* parent = nullptr);
    ~GitDiffGutter() override;

    // Registra o editor, configura markers/margin e dispara diff inicial.
    // No-op se editor for nullptr.
    void attach(ScintillaEdit* editor, const QString& filePath);

    // Remove markers e desfaz binding. No-op se editor não estiver attached.
    void detach(ScintillaEdit* editor);

    // Re-roda `git diff` pra editor/filePath. Respeita coalescência.
    void refresh(ScintillaEdit* editor, const QString& filePath);

    // Liga/desliga o gutter globalmente. Persiste em QSettings.
    void setEnabled(bool on);
    bool isEnabled() const;

private:
    // Estado por editor attached.
    struct Binding {
        QPointer<ScintillaEdit> editor;
        QString filePath;       // path absoluto do arquivo
        QTimer*  debounce = nullptr;
    };

    // Estado por query `git diff` em voo (1 por path).
    struct PendingQuery {
        QString filePath;
        QString workingDir;     // dirname(filePath)
        QString repoRoot;       // resolvido na etapa 1
        QString relPath;        // relativo a repoRoot
        int     stage = 0;      // 0=rev-parse, 1=diff
        QProcess* proc = nullptr;
        QTimer*   timeout = nullptr;
    };

    // Marker types após o parsing da hunk.
    enum class HunkKind { Added, Modified, RemovedAbove };
    struct LineMark {
        int line = 0;           // 0-based
        HunkKind kind = HunkKind::Added;
    };

    // Configura margin + markers no editor (idempotente).
    void configureEditor(ScintillaEdit* editor);

    // Limpa todos os markers de diff no editor.
    void clearMarkers(ScintillaEdit* editor);

    // Aplica uma lista de marcas (substitui o set anterior).
    void applyMarks(ScintillaEdit* editor, const QList<LineMark>& marks);

    // Dispara a próxima etapa do pipeline (rev-parse -> diff).
    void startStage(PendingQuery* q);
    void onProcessFinished(PendingQuery* q, int exitCode, int exitStatus);
    void onTimeout(PendingQuery* q);
    void finalizeWith(PendingQuery* q, const QList<LineMark>& marks);

    // Parser de hunks. Retorna lista de LineMark a partir do output do diff.
    static QList<LineMark> parseDiff(const QString& diffOutput);

    // Helpers internos.
    Binding* findBinding(ScintillaEdit* editor);
    Binding* findBindingByPath(const QString& path);

    // Dispara a query coalescida pra um path específico.
    void scheduleQuery(const QString& filePath);

    QList<Binding*> m_bindings;
    QHash<QString, PendingQuery*> m_inFlight;
    bool m_enabled = true;
};
