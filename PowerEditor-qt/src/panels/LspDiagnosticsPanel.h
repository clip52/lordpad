#pragma once

#include <QDockWidget>
#include <QHash>
#include <QString>

#include "../LspClient.h"

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QToolButton;
class QComboBox;

// Dockable panel that displays LSP diagnostics for the current file
// (errors / warnings / info / hints). Read-only — it only renders what
// LspClient already produced and emits a signal when the user double-clicks
// an entry so the host (MainWindow) can navigate the editor caret.
class LspDiagnosticsPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit LspDiagnosticsPanel(QWidget* parent = nullptr);

    // Replace the diagnostics rendered for `filePath`. Pass an empty list to clear
    // diagnostics for that file. The panel keeps an internal cache keyed by file path
    // so it can re-render the active file's diagnostics on demand.
    void setDiagnostics(const QString& filePath, const QList<LspDiagnostic>& diags);

    // Switch which file's diagnostics are shown. Pass empty to show "no file".
    void setActiveFile(const QString& filePath);

    // Show a one-shot status message (e.g. "Servidor X não encontrado — instale com: ...").
    void showStatusMessage(const QString& text);

signals:
    void diagnosticActivated(const QString& filePath, int line, int column);

private slots:
    void onItemActivated(QTreeWidgetItem* item, int column);
    void onSeverityFilterChanged(int index);
    void onClearClicked();

private:
    void rebuildTree();

    QTreeWidget* m_tree   = nullptr;
    QLabel*      m_status = nullptr;
    QComboBox*   m_filter = nullptr;
    QToolButton* m_clearBtn = nullptr;

    QString m_activeFile;
    QHash<QString, QList<LspDiagnostic>> m_cache;
    int m_severityFilter = 0; // 0=all, 1=errors+warnings, 2=errors only
};
