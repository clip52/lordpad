#pragma once

#include <QDockWidget>

class QLineEdit;
class QPushButton;
class QTableWidget;
class QLabel;

// EnvManagerPanel (M58) — visualiza/edita arquivos .env como tabela.
class EnvManagerPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit EnvManagerPanel(QWidget* parent = nullptr);
private slots:
    void onLoad();
    void onSave();
    void onAddRow();
    void onDelRow();
    void onPickFile();
private:
    QString render() const;

    QLineEdit*    m_pathEdit = nullptr;
    QPushButton*  m_pickBtn  = nullptr;
    QPushButton*  m_loadBtn  = nullptr;
    QPushButton*  m_saveBtn  = nullptr;
    QPushButton*  m_addBtn   = nullptr;
    QPushButton*  m_delBtn   = nullptr;
    QTableWidget* m_table    = nullptr;
    QLabel*       m_status   = nullptr;
};
