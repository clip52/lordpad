#pragma once

#include <QDockWidget>
#include <QString>

class QListWidget;
class QListWidgetItem;
class QLineEdit;
class QPushButton;

// TodoPanel — utilitário M12. Lista de tarefas global (não por workspace),
// persistida em QSettings ("Todo/items" como QStringList no formato
// "[x] texto" ou "[ ] texto"). Adicionar via QLineEdit + Enter, marcar via
// duplo-clique ou checkbox, apagar via Delete.
class TodoPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit TodoPanel(QWidget* parent = nullptr);

private slots:
    void onAdd();
    void onItemChanged(QListWidgetItem* it);
    void onDelete();
    void onClearDone();

private:
    void load();
    void persist() const;

    QListWidget* m_list = nullptr;
    QLineEdit*   m_input = nullptr;
    QPushButton* m_addBtn = nullptr;
    QPushButton* m_delBtn = nullptr;
    QPushButton* m_clearDoneBtn = nullptr;
    bool         m_suppressItemSignals = false;
};
