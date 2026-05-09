#pragma once

#include <QDockWidget>

class QLabel;
class QPushButton;
class QPlainTextEdit;
class QTableWidget;
class ScintillaEdit;

// TextStatsPanel (M36) — count chars/words/lines + top word frequency.
class TextStatsPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit TextStatsPanel(QWidget* parent = nullptr);
    void setActiveEditor(ScintillaEdit* ed) { m_editor = ed; }
private slots:
    void onAnalyze();
private:
    ScintillaEdit*  m_editor = nullptr;
    QLabel*         m_summary = nullptr;
    QTableWidget*   m_freq = nullptr;
    QPushButton*    m_btn = nullptr;
};
