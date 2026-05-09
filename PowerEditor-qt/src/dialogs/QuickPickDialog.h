#pragma once

#include <QDialog>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QList>

class QListWidget;
class QLineEdit;

// Lightweight modal picker reused by M9 LSP UI for find-references,
// document-symbols, and workspace-symbols. Each row carries a primary
// label, an optional subtitle (rendered greyed) and an opaque QVariant
// payload (e.g. an LspLocation or LspSymbol struct embedded in QVariant).
//
// Filtering is case-insensitive substring on label+subtitle.
class QuickPickDialog : public QDialog {
    Q_OBJECT
public:
    struct Item {
        QString  label;
        QString  subtitle;
        QVariant data;
    };

    QuickPickDialog(QWidget* parent, const QString& title, const QList<Item>& items);

    // After exec(), returns the picked item's data or an invalid QVariant on cancel.
    QVariant pickedData() const { return m_picked; }

    // For workspace-symbols where the search is server-driven: lets the host
    // listen to the filter line so it can re-query as the user types.
    void setLiveQueryMode(bool on);
    QString currentQuery() const;
    void replaceItems(const QList<Item>& items);

signals:
    void queryChanged(const QString& text);

private slots:
    void onAccept();
    void onFilterChanged(const QString& text);

private:
    QListWidget* m_list = nullptr;
    QLineEdit*   m_filter = nullptr;
    QList<Item>  m_items;
    QVariant     m_picked;
    bool         m_liveQuery = false;
};
