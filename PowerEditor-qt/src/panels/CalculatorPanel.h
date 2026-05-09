#pragma once

#include <QDockWidget>
#include <QString>

class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QLabel;

// CalculatorPanel — utilitário M12. Expressões matemáticas avaliadas com um
// parser próprio (shunting-yard → eval). Suporta:
//   operadores: + - * / % ^   (unary -/+, '^' é potência)
//   funções:    sqrt, sin, cos, tan, asin, acos, atan, log (base 10),
//               ln (base e), exp, abs, floor, ceil, round
//   constantes: pi, e
//   parênteses
// O painel mostra o resultado abaixo do input e mantém um histórico
// persistido em QSettings (até 50 entradas).
class CalculatorPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit CalculatorPanel(QWidget* parent = nullptr);

    // Avalia `expr`. Retorna true e preenche outValue em sucesso; em falha
    // retorna false e preenche outError com mensagem amigável.
    static bool evaluate(const QString& expr, double& outValue, QString* outError = nullptr);

private slots:
    void onEvaluate();
    void onHistoryActivated();

private:
    void appendHistory(const QString& expr, double value);
    void rebuildHistory();
    void persistHistory() const;
    void loadHistory();

    QLineEdit*      m_input  = nullptr;
    QLabel*         m_result = nullptr;
    QPushButton*    m_eval   = nullptr;
    QPlainTextEdit* m_history = nullptr;
    QStringList     m_historyList;   // "expr = result" entries, newest first
};
