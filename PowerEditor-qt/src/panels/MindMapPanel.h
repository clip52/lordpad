#pragma once

#include <QDockWidget>
#include <QString>

class QGraphicsScene;
class QGraphicsView;
class QPushButton;
class QLineEdit;
class QLabel;

// MindMapPanel (M37) — mind map text-based: usuário digita árvore com
// indentação por tab, painel renderiza como nodes/edges via QGraphicsScene.
class MindMapPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit MindMapPanel(QWidget* parent = nullptr);

private slots:
    void onRender();
    void onSavePng();

private:
    QGraphicsScene* m_scene = nullptr;
    QGraphicsView*  m_view  = nullptr;
    class QPlainTextEdit* m_source = nullptr;
    QPushButton*    m_renderBtn = nullptr;
    QPushButton*    m_saveBtn = nullptr;
};
