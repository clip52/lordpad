#include "SqlitePanel.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTableView>
#include <QtSql/QSqlError>
#include <QtSql/QSqlQuery>
#include <QtSql/QSqlQueryModel>
#include <QVBoxLayout>
#include <QWidget>

SqlitePanel::SqlitePanel(QWidget* parent) : QDockWidget(tr("SQLite"), parent)
{
    setObjectName(QStringLiteral("SqlitePanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);

    m_openBtn   = new QPushButton(tr("Abrir .db…"), root);
    m_pathLabel = new QLabel(tr("(nenhum banco aberto)"), root);
    m_pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto* topRow = new QHBoxLayout();
    topRow->addWidget(m_openBtn);
    topRow->addWidget(m_pathLabel, 1);

    m_tables = new QListWidget(root);
    m_tables->setSelectionMode(QAbstractItemView::SingleSelection);

    m_sql = new QPlainTextEdit(root);
    m_sql->setPlaceholderText(tr("SELECT * FROM tabela LIMIT 100;"));
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(10);
    m_sql->setFont(mono);
    m_runBtn = new QPushButton(tr("Rodar (Ctrl+Enter)"), root);

    m_results = new QTableView(root);
    m_results->horizontalHeader()->setStretchLastSection(true);
    m_results->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_model = new QSqlQueryModel(this);
    m_results->setModel(m_model);

    m_status = new QLabel(root);

    auto* rightWrap = new QWidget(root);
    auto* rightLay  = new QVBoxLayout(rightWrap);
    rightLay->setContentsMargins(0, 0, 0, 0);
    rightLay->addWidget(m_sql, 1);
    rightLay->addWidget(m_runBtn);
    rightLay->addWidget(m_results, 2);

    auto* split = new QSplitter(Qt::Horizontal, root);
    split->addWidget(m_tables);
    split->addWidget(rightWrap);
    split->setStretchFactor(0, 1);
    split->setStretchFactor(1, 4);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(topRow);
    lay->addWidget(split, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_openBtn, &QPushButton::clicked, this, &SqlitePanel::onOpenDb);
    connect(m_runBtn,  &QPushButton::clicked, this, &SqlitePanel::onRunQuery);
    connect(m_tables,  &QListWidget::itemActivated, this, &SqlitePanel::onTableActivated);

    auto* runShortcut = new QAction(this);
    runShortcut->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return));
    connect(runShortcut, &QAction::triggered, this, &SqlitePanel::onRunQuery);
    m_sql->addAction(runShortcut);
}

SqlitePanel::~SqlitePanel()
{
    if (m_db.isOpen()) m_db.close();
    if (!m_db.connectionName().isEmpty())
        QSqlDatabase::removeDatabase(m_db.connectionName());
}

void SqlitePanel::onOpenDb()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Abrir banco SQLite"),
        QString(), tr("SQLite (*.db *.sqlite *.sqlite3 *.db3)"));
    if (path.isEmpty()) return;

    if (m_db.isOpen()) m_db.close();
    if (!m_db.connectionName().isEmpty())
        QSqlDatabase::removeDatabase(m_db.connectionName());

    const QString connName = QStringLiteral("npp-qt-sqlite-%1").arg(reinterpret_cast<qulonglong>(this));
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
    m_db.setDatabaseName(path);
    if (!m_db.open()) {
        QMessageBox::warning(this, tr("SQLite"),
            tr("Falha ao abrir: %1").arg(m_db.lastError().text()));
        return;
    }
    m_dbPath = path;
    m_pathLabel->setText(path);
    rebuildTablesList();
    m_status->setText(tr("OK"));
}

void SqlitePanel::rebuildTablesList()
{
    m_tables->clear();
    if (!m_db.isOpen()) return;
    QSqlQuery q(m_db);
    if (!q.exec(QStringLiteral(
            "SELECT name FROM sqlite_master "
            "WHERE type IN ('table','view') AND name NOT LIKE 'sqlite_%' "
            "ORDER BY name"))) {
        m_status->setText(tr("Erro: %1").arg(q.lastError().text()));
        return;
    }
    while (q.next()) m_tables->addItem(q.value(0).toString());
}

void SqlitePanel::onTableActivated(QListWidgetItem* it)
{
    if (!it) return;
    m_sql->setPlainText(QStringLiteral("SELECT * FROM \"%1\" LIMIT 200;").arg(it->text()));
    onRunQuery();
}

void SqlitePanel::onRunQuery()
{
    if (!m_db.isOpen()) {
        m_status->setText(tr("Abra um banco primeiro."));
        return;
    }
    const QString sql = m_sql->toPlainText().trimmed();
    if (sql.isEmpty()) return;
    m_model->setQuery(sql, m_db);
    if (m_model->lastError().isValid()) {
        m_status->setText(tr("Erro: %1").arg(m_model->lastError().text()));
        return;
    }
    // Force model to fetch all rows so row count is honest in the status bar.
    while (m_model->canFetchMore()) m_model->fetchMore();
    m_status->setText(tr("%1 linha(s)").arg(m_model->rowCount()));
}
