#pragma once

#include <QDockWidget>
#include <QStringList>

class QListWidget;
class QListWidgetItem;
class QTextBrowser;
class QLineEdit;
class QPushButton;
class QLabel;

// CheatsheetPanel (M60) — viewer Markdown de diretório configurável.
class CheatsheetPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit CheatsheetPanel(QWidget* parent = nullptr);
public slots:
    void reload();
private slots:
    void onPickDir();
    void onItemActivated(QListWidgetItem* it);
    void onFilterChanged(const QString& text);
private:
    QString markdownToHtml(const QString& md) const;

    QLineEdit*    m_dirEdit = nullptr;
    QPushButton*  m_pickBtn = nullptr;
    QPushButton*  m_reloadBtn = nullptr;
    QLineEdit*    m_filter  = nullptr;
    QListWidget*  m_list = nullptr;
    QTextBrowser* m_view = nullptr;
    QLabel*       m_status = nullptr;
};
