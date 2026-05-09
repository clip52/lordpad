#pragma once
#include <QDockWidget>
#include <QList>
#include <QString>
#include <QModelIndex>

#include "../LspClient.h"   // LspSymbol — small struct, header-light

class ScintillaEdit;
class QListView;
class QStandardItemModel;
class QLineEdit;
class QTimer;

class FunctionListPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit FunctionListPanel(QWidget* parent = nullptr);

    // Bind to the active editor; calling with nullptr clears the list.
    // Pass the lexer name (e.g. "cpp", "python", "javascript") so we use the right regex set.
    // If lexerName is empty, the panel falls back to a generic regex covering common patterns.
    void setActiveEditor(ScintillaEdit* editor, const QString& lexerName);

    // Recompute list from the current editor's text. Caller invokes after edits.
    void refresh();

    // M13: replace the list with LSP-provided document symbols. Each entry is
    // shown as `containerName.name` so hierarchy stays readable in the flat
    // QListView. Calling with an empty list reverts to the regex-based outline
    // produced by the next refresh().
    void setLspSymbols(const QList<LspSymbol>& syms);

signals:
    void gotoLineRequested(int line);   // 1-based; emitted on item activation

private slots:
    void onFilterTextChanged(const QString& text);
    void onItemActivated(const QModelIndex& index);
    void onEditorModified();
    void onRefreshClicked();

private:
    void applyFilter();

    ScintillaEdit*       m_editor   = nullptr;
    QString              m_lexerName;
    QListView*           m_listView = nullptr;
    QStandardItemModel*  m_model    = nullptr;
    QLineEdit*           m_filterEdit = nullptr;
    QTimer*              m_debounce = nullptr;
    QMetaObject::Connection m_modifiedConn;
};
