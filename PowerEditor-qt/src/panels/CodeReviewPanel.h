#pragma once

#include <QDockWidget>
#include <QString>

class QTreeWidget;
class QTreeWidgetItem;
class QPlainTextEdit;
class QPushButton;
class QLabel;

// CodeReviewPanel (M30) — inline comments por arquivo, persistidos em
// QSettings/CodeReview/<filePath> como JSON {line, text}[].
class CodeReviewPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit CodeReviewPanel(QWidget* parent = nullptr);

    void setActiveFile(const QString& path);

signals:
    void gotoLineRequested(int line);

private slots:
    void onAdd();
    void onDelete();
    void onItemActivated(QTreeWidgetItem* it, int col);

private:
    void loadForActive();
    void persistForActive();

    QString         m_path;
    QTreeWidget*    m_list = nullptr;
    QPlainTextEdit* m_text = nullptr;
    QPushButton*    m_addBtn = nullptr;
    QPushButton*    m_delBtn = nullptr;
    QLabel*         m_status = nullptr;
};
