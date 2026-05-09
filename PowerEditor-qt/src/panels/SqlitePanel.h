#pragma once

#include <QDockWidget>
#include <QString>

#include <QtSql/QSqlDatabase>

class QListWidget;
class QPlainTextEdit;
class QTableView;
class QPushButton;
class QLabel;
class QSqlQueryModel;

// SqlitePanel (M15) — small SQLite browser/runner using QtSql with QSQLITE.
//
// UI: top bar with "Abrir .db…" + current path; left list with table names;
// right side: SQL editor + Run button + result table. Default click on a
// table generates `SELECT * FROM <name> LIMIT 200;` so the user can poke at
// data without typing.
class SqlitePanel : public QDockWidget {
    Q_OBJECT
public:
    explicit SqlitePanel(QWidget* parent = nullptr);
    ~SqlitePanel() override;

private slots:
    void onOpenDb();
    void onRunQuery();
    void onTableActivated(class QListWidgetItem* it);

private:
    void rebuildTablesList();

    QString          m_dbPath;
    QSqlDatabase     m_db;
    QSqlQueryModel*  m_model = nullptr;

    QPushButton*     m_openBtn = nullptr;
    QLabel*          m_pathLabel = nullptr;
    QListWidget*     m_tables = nullptr;
    QPlainTextEdit*  m_sql = nullptr;
    QPushButton*     m_runBtn = nullptr;
    QTableView*      m_results = nullptr;
    QLabel*          m_status = nullptr;
};
