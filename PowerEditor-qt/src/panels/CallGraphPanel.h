#pragma once

#include <QDockWidget>

class QGraphicsScene;
class QGraphicsView;
class QPushButton;
class QLabel;
class ScintillaEdit;

// CallGraphPanel (M41) — render simples de call graph: detecta `def name(`
// (Python) ou `name(` (cpp/c) no buffer ativo, varre quem chama quem,
// desenha como nodes/arrows.
class CallGraphPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit CallGraphPanel(QWidget* parent = nullptr);
    void setActiveEditor(ScintillaEdit* ed) { m_editor = ed; }
private slots:
    void onAnalyze();
private:
    ScintillaEdit* m_editor = nullptr;
    QPushButton*   m_btn = nullptr;
    QGraphicsScene* m_scene = nullptr;
    QGraphicsView*  m_view = nullptr;
    QLabel*        m_status = nullptr;
};
