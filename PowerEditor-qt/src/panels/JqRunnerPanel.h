#pragma once

#include <QDockWidget>

class QLineEdit;
class QPushButton;
class QPlainTextEdit;
class QLabel;
class ScintillaEdit;

// JqRunnerPanel (M55) — roda jq query no buffer JSON ativo.
class JqRunnerPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit JqRunnerPanel(QWidget* parent = nullptr);
    void setActiveEditor(ScintillaEdit* ed) { m_editor = ed; }
private slots:
    void onRun();
    void onWriteBuffer();
private:
    ScintillaEdit*  m_editor = nullptr;
    QLineEdit*      m_query  = nullptr;
    QPushButton*    m_runBtn = nullptr;
    QPushButton*    m_writeBtn = nullptr;
    QPlainTextEdit* m_out    = nullptr;
    QLabel*         m_status = nullptr;
};
