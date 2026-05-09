#pragma once

#include <QDockWidget>
#include <QString>

class QComboBox;
class QLineEdit;
class QPushButton;
class QPlainTextEdit;
class QTreeWidget;
class QLabel;

// ProfileRunnerPanel (M31) — front-end pra py-spy (Python) ou perf (Linux).
//
// Modo combo: py-spy / perf.
// py-spy: roda `py-spy record -d <secs> -o /tmp/x.svg -- <cmd>` ou
//         `py-spy top --pid <PID>` capturando stacks num intervalo.
// perf:   roda `perf record -F 99 -g -- <cmd>` + `perf report --stdio`.
//
// O painel não tenta desenhar flame graph — exibe a saída em árvore textual
// (fold count agrupado) que é honesta e útil pra ver hot paths.
class ProfileRunnerPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit ProfileRunnerPanel(QWidget* parent = nullptr);

private slots:
    void onRun();

private:
    QComboBox*      m_provider = nullptr;
    QLineEdit*      m_command  = nullptr;
    QLineEdit*      m_duration = nullptr;
    QPlainTextEdit* m_output = nullptr;
    QPushButton*    m_runBtn = nullptr;
    QLabel*         m_status = nullptr;
};
