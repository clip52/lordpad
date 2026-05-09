#pragma once

#include <QDockWidget>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QLabel;

// DbShellPanel (M23) — frontend pra `psql` ou `redis-cli` via subprocess.
//
// Modo combo: escolhe Postgres / Redis. Conecta com URL/host. Comando
// editor + Run dispara o subprocess one-shot, captura output. É shell-side,
// não persistent connection — cada Run é uma invocação. Bom o suficiente
// pra queries ad-hoc, ruim pra transações / sessions.
class DbShellPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit DbShellPanel(QWidget* parent = nullptr);

private slots:
    void onRun();

private:
    QComboBox*      m_provider = nullptr;
    QLineEdit*      m_conn = nullptr;
    QPlainTextEdit* m_query = nullptr;
    QPlainTextEdit* m_output = nullptr;
    QPushButton*    m_runBtn = nullptr;
    QLabel*         m_status = nullptr;
};
