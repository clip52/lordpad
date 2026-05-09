#pragma once

#include <QDockWidget>
#include <QPointer>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QLabel;
class QProcess;

// CliShellPanel (M47) — wrapper genérico p/ mysql/mongosh/redis-cli/psql via subprocess.
// Útil quando o driver QtSql não está disponível ou pra Mongo (sem driver Qt).
class CliShellPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit CliShellPanel(QWidget* parent = nullptr);
private slots:
    void onConnect();
    void onRun();
    void onClear();
private:
    QString flavorTool() const;
    QStringList flavorArgs() const;

    QComboBox*       m_flavor = nullptr;   // mysql / mongosh / redis-cli / psql
    QLineEdit*       m_uri    = nullptr;
    QPlainTextEdit*  m_query  = nullptr;
    QPlainTextEdit*  m_out    = nullptr;
    QPushButton*     m_connectBtn = nullptr;
    QPushButton*     m_runBtn = nullptr;
    QPushButton*     m_clearBtn = nullptr;
    QLabel*          m_status = nullptr;
    QPointer<QProcess> m_proc;
};
