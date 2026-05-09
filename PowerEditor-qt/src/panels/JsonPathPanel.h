#pragma once

#include <QDockWidget>

class QLineEdit;
class QPlainTextEdit;
class QLabel;
class ScintillaEdit;

// JsonPathPanel (M18) — minimal jq-lite. Aceita paths .a.b[2].c[*].d.
// Carrega buffer ativo como JSON, aplica path, mostra resultado JSON.
class JsonPathPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit JsonPathPanel(QWidget* parent = nullptr);

    void setActiveEditor(ScintillaEdit* editor);

private slots:
    void onApply();

private:
    ScintillaEdit*  m_editor = nullptr;
    QLineEdit*      m_path = nullptr;
    QPlainTextEdit* m_output = nullptr;
    QLabel*         m_status = nullptr;
};
