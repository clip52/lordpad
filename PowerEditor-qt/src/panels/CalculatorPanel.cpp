#include "CalculatorPanel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QWidget>

#include <cmath>
#include <cstdlib>
#include <vector>

namespace {

// =============================================================================
// Tiny shunting-yard expression evaluator. Plenty of math libraries do this
// better — but bringing one in is overkill for a calculator dock. ~150 LOC of
// fully self-contained code is the right tradeoff.
// =============================================================================
struct Token {
    enum Kind { Num, Op, LParen, RParen, Func, Comma };
    Kind     kind = Num;
    double   value = 0.0;
    QString  text;             // operator symbol or function name
    int      precedence = 0;
    bool     rightAssoc = false;
    int      argCount = 1;
};

// op precedence lookup
int prec(const QString& op) {
    if (op == "+" || op == "-")           return 1;
    if (op == "*" || op == "/" || op == "%") return 2;
    if (op == "^")                        return 3;
    if (op == "u-" || op == "u+")         return 4;   // unary
    return 0;
}
bool rightAssoc(const QString& op) {
    return op == "^" || op == "u-" || op == "u+";
}
bool isFunction(const QString& s) {
    static const QStringList fns = {
        "sqrt","sin","cos","tan","asin","acos","atan",
        "log","ln","exp","abs","floor","ceil","round"
    };
    return fns.contains(s);
}

double applyFunction(const QString& fn, double x, bool& ok) {
    ok = true;
    if (fn == "sqrt") { if (x < 0) { ok = false; return 0; } return std::sqrt(x); }
    if (fn == "sin")   return std::sin(x);
    if (fn == "cos")   return std::cos(x);
    if (fn == "tan")   return std::tan(x);
    if (fn == "asin")  return std::asin(x);
    if (fn == "acos")  return std::acos(x);
    if (fn == "atan")  return std::atan(x);
    if (fn == "log")   { if (x <= 0) { ok = false; return 0; } return std::log10(x); }
    if (fn == "ln")    { if (x <= 0) { ok = false; return 0; } return std::log(x); }
    if (fn == "exp")   return std::exp(x);
    if (fn == "abs")   return std::fabs(x);
    if (fn == "floor") return std::floor(x);
    if (fn == "ceil")  return std::ceil(x);
    if (fn == "round") return std::round(x);
    ok = false;
    return 0;
}

double applyOperator(const QString& op, double a, double b, bool& ok) {
    ok = true;
    if (op == "+") return a + b;
    if (op == "-") return a - b;
    if (op == "*") return a * b;
    if (op == "/") { if (b == 0.0) { ok = false; return 0; } return a / b; }
    if (op == "%") { if (b == 0.0) { ok = false; return 0; } return std::fmod(a, b); }
    if (op == "^") return std::pow(a, b);
    ok = false;
    return 0;
}

bool tokenize(const QString& s, std::vector<Token>& out, QString* err)
{
    int i = 0;
    bool prevWasOperand = false;
    while (i < s.size()) {
        const QChar c = s.at(i);
        if (c.isSpace()) { ++i; continue; }
        if (c.isDigit() || c == '.') {
            int j = i;
            while (j < s.size() && (s.at(j).isDigit() || s.at(j) == '.')) ++j;
            // optional scientific exponent: e+10, e-3, e5
            if (j < s.size() && (s.at(j) == 'e' || s.at(j) == 'E')) {
                ++j;
                if (j < s.size() && (s.at(j) == '+' || s.at(j) == '-')) ++j;
                while (j < s.size() && s.at(j).isDigit()) ++j;
            }
            bool ok;
            const double v = s.mid(i, j - i).toDouble(&ok);
            if (!ok) { if (err) *err = QObject::tr("Número inválido."); return false; }
            Token t; t.kind = Token::Num; t.value = v;
            out.push_back(t);
            prevWasOperand = true;
            i = j;
            continue;
        }
        if (c.isLetter()) {
            int j = i;
            while (j < s.size() && (s.at(j).isLetterOrNumber() || s.at(j) == '_')) ++j;
            const QString id = s.mid(i, j - i);
            if (id == "pi")        { Token t; t.kind = Token::Num; t.value = M_PI;     out.push_back(t); prevWasOperand = true; }
            else if (id == "e")    { Token t; t.kind = Token::Num; t.value = M_E;      out.push_back(t); prevWasOperand = true; }
            else if (isFunction(id)) {
                Token t; t.kind = Token::Func; t.text = id;
                out.push_back(t);
                prevWasOperand = false;
            } else {
                if (err) *err = QObject::tr("Identificador desconhecido: %1").arg(id);
                return false;
            }
            i = j;
            continue;
        }
        if (c == '(') { Token t; t.kind = Token::LParen; out.push_back(t); prevWasOperand = false; ++i; continue; }
        if (c == ')') { Token t; t.kind = Token::RParen; out.push_back(t); prevWasOperand = true;  ++i; continue; }
        if (c == ',') { Token t; t.kind = Token::Comma;  out.push_back(t); prevWasOperand = false; ++i; continue; }
        // Operators
        const QString opS = QString(c);
        if (opS == "+" || opS == "-") {
            Token t; t.kind = Token::Op;
            t.text = prevWasOperand ? opS : (opS == "-" ? QStringLiteral("u-") : QStringLiteral("u+"));
            t.precedence = prec(t.text);
            t.rightAssoc = rightAssoc(t.text);
            out.push_back(t);
            prevWasOperand = false;
            ++i;
            continue;
        }
        if (opS == "*" || opS == "/" || opS == "%" || opS == "^") {
            Token t; t.kind = Token::Op; t.text = opS;
            t.precedence = prec(t.text);
            t.rightAssoc = rightAssoc(t.text);
            out.push_back(t);
            prevWasOperand = false;
            ++i;
            continue;
        }
        if (err) *err = QObject::tr("Caractere inesperado: %1").arg(c);
        return false;
    }
    return true;
}

bool toRpn(const std::vector<Token>& tokens, std::vector<Token>& rpn, QString* err)
{
    std::vector<Token> stack;
    for (const Token& t : tokens) {
        switch (t.kind) {
            case Token::Num:  rpn.push_back(t); break;
            case Token::Func: stack.push_back(t); break;
            case Token::Comma:
                while (!stack.empty() && stack.back().kind != Token::LParen) {
                    rpn.push_back(stack.back()); stack.pop_back();
                }
                break;
            case Token::Op:
                while (!stack.empty() && stack.back().kind != Token::LParen
                       && (stack.back().kind == Token::Func
                           || stack.back().precedence > t.precedence
                           || (stack.back().precedence == t.precedence && !t.rightAssoc))) {
                    rpn.push_back(stack.back()); stack.pop_back();
                }
                stack.push_back(t);
                break;
            case Token::LParen: stack.push_back(t); break;
            case Token::RParen:
                while (!stack.empty() && stack.back().kind != Token::LParen) {
                    rpn.push_back(stack.back()); stack.pop_back();
                }
                if (stack.empty()) { if (err) *err = QObject::tr("Parêntese sem par."); return false; }
                stack.pop_back();   // discard '('
                if (!stack.empty() && stack.back().kind == Token::Func) {
                    rpn.push_back(stack.back()); stack.pop_back();
                }
                break;
        }
    }
    while (!stack.empty()) {
        if (stack.back().kind == Token::LParen) {
            if (err) *err = QObject::tr("Parêntese sem par."); return false;
        }
        rpn.push_back(stack.back()); stack.pop_back();
    }
    return true;
}

bool evalRpn(const std::vector<Token>& rpn, double& out, QString* err)
{
    std::vector<double> st;
    for (const Token& t : rpn) {
        if (t.kind == Token::Num) { st.push_back(t.value); continue; }
        if (t.kind == Token::Func) {
            if (st.empty()) { if (err) *err = QObject::tr("Argumento ausente."); return false; }
            const double x = st.back(); st.pop_back();
            bool ok = false;
            const double v = applyFunction(t.text, x, ok);
            if (!ok) { if (err) *err = QObject::tr("Domínio inválido em %1.").arg(t.text); return false; }
            st.push_back(v);
            continue;
        }
        if (t.kind == Token::Op) {
            if (t.text == "u-" || t.text == "u+") {
                if (st.empty()) { if (err) *err = QObject::tr("Operando ausente."); return false; }
                const double a = st.back(); st.pop_back();
                st.push_back(t.text == "u-" ? -a : a);
                continue;
            }
            if (st.size() < 2) { if (err) *err = QObject::tr("Operando ausente."); return false; }
            const double b = st.back(); st.pop_back();
            const double a = st.back(); st.pop_back();
            bool ok = false;
            const double v = applyOperator(t.text, a, b, ok);
            if (!ok) { if (err) *err = QObject::tr("Erro aritmético."); return false; }
            st.push_back(v);
            continue;
        }
    }
    if (st.size() != 1) { if (err) *err = QObject::tr("Expressão malformada."); return false; }
    out = st.back();
    return true;
}

} // namespace

bool CalculatorPanel::evaluate(const QString& expr, double& outValue, QString* outError)
{
    std::vector<Token> tokens, rpn;
    if (!tokenize(expr, tokens, outError)) return false;
    if (tokens.empty()) { if (outError) *outError = QObject::tr("Expressão vazia."); return false; }
    if (!toRpn(tokens, rpn, outError))     return false;
    return evalRpn(rpn, outValue, outError);
}

CalculatorPanel::CalculatorPanel(QWidget* parent)
    : QDockWidget(tr("Calculadora"), parent)
{
    setObjectName(QStringLiteral("CalculatorPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    m_input  = new QLineEdit(root);
    m_input->setPlaceholderText(tr("ex.: 2*sin(pi/4)+sqrt(16) — Enter avalia"));
    m_eval   = new QPushButton(tr("="), root);
    m_result = new QLabel(root);
    m_result->setStyleSheet(QStringLiteral("QLabel { font-size: 14pt; font-weight: bold; }"));
    m_result->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_history = new QPlainTextEdit(root);
    m_history->setReadOnly(true);
    m_history->setMaximumBlockCount(50);

    auto* row = new QHBoxLayout();
    row->addWidget(m_input, 1);
    row->addWidget(m_eval);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row);
    lay->addWidget(m_result);
    lay->addWidget(new QLabel(tr("Histórico:"), root));
    lay->addWidget(m_history, 1);
    setWidget(root);

    connect(m_input, &QLineEdit::returnPressed, this, &CalculatorPanel::onEvaluate);
    connect(m_eval,  &QPushButton::clicked,     this, &CalculatorPanel::onEvaluate);

    loadHistory();
}

void CalculatorPanel::onEvaluate()
{
    const QString expr = m_input->text().trimmed();
    if (expr.isEmpty()) return;
    double v = 0.0;
    QString err;
    if (!evaluate(expr, v, &err)) {
        m_result->setText(tr("× %1").arg(err));
        return;
    }
    // Format the result with adaptive precision: show 12 digits, then strip
    // trailing zeros to keep it readable.
    QString out = QString::number(v, 'g', 12);
    m_result->setText(QStringLiteral("= ") + out);
    appendHistory(expr, v);
}

void CalculatorPanel::onHistoryActivated() { /* not used; history is text-only */ }

void CalculatorPanel::appendHistory(const QString& expr, double value)
{
    const QString line = QStringLiteral("%1 = %2").arg(expr, QString::number(value, 'g', 12));
    m_historyList.prepend(line);
    while (m_historyList.size() > 50) m_historyList.removeLast();
    persistHistory();
    rebuildHistory();
}

void CalculatorPanel::rebuildHistory()
{
    m_history->setPlainText(m_historyList.join('\n'));
}

void CalculatorPanel::persistHistory() const
{
    QSettings s;
    s.setValue(QStringLiteral("Calculator/history"), m_historyList);
}

void CalculatorPanel::loadHistory()
{
    QSettings s;
    m_historyList = s.value(QStringLiteral("Calculator/history")).toStringList();
    rebuildHistory();
}
