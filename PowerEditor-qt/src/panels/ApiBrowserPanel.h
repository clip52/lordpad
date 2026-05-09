#pragma once

#include <QDockWidget>

class QLineEdit;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QPlainTextEdit;
class QLabel;

// ApiBrowserPanel (M69) — carrega OpenAPI/Swagger (json) e mostra paths/operações.
class ApiBrowserPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit ApiBrowserPanel(QWidget* parent = nullptr);
private slots:
    void onPickFile();
    void onLoad();
    void onItemActivated(QTreeWidgetItem* it);
private:
    QLineEdit*      m_pathEdit = nullptr;
    QPushButton*    m_pickBtn = nullptr;
    QPushButton*    m_loadBtn = nullptr;
    QTreeWidget*    m_tree = nullptr;
    QPlainTextEdit* m_detail = nullptr;
    QLabel*         m_status = nullptr;
};
