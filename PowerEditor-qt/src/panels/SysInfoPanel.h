#pragma once

#include <QDockWidget>

class QPlainTextEdit;
class QPushButton;
class QTimer;

// SysInfoPanel (M20) — exibe informações do sistema (uname, mem, cpu count,
// disk free, processo do editor) atualizadas a cada 5s. É read-only — pra ops
// de verdade use o terminal embutido.
class SysInfoPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit SysInfoPanel(QWidget* parent = nullptr);

private slots:
    void onRefresh();

private:
    QPlainTextEdit* m_text = nullptr;
    QPushButton*    m_refreshBtn = nullptr;
    QTimer*         m_timer = nullptr;
};
