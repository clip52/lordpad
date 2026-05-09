#pragma once

#include <QDockWidget>
#include <QStringList>

class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QPlainTextEdit;
class QLabel;
class QNetworkAccessManager;

// RssReaderPanel (M46) — fetcha feeds RSS/Atom, lista items, abre body.
class RssReaderPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit RssReaderPanel(QWidget* parent = nullptr);
private slots:
    void onAddFeed();
    void onRemoveFeed();
    void onFetchAll();
    void onFeedActivated(QListWidgetItem* it);
    void onItemActivated(QListWidgetItem* it);
private:
    void persist() const;
    void load();

    QLineEdit*    m_urlEdit = nullptr;
    QPushButton*  m_addBtn = nullptr;
    QPushButton*  m_delBtn = nullptr;
    QPushButton*  m_fetchBtn = nullptr;
    QListWidget*  m_feeds = nullptr;
    QListWidget*  m_items = nullptr;
    QPlainTextEdit* m_body = nullptr;
    QLabel*       m_status = nullptr;
    QNetworkAccessManager* m_nam = nullptr;
};
