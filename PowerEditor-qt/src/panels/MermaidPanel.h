#pragma once

#include <QDockWidget>
#include <QString>

class QPlainTextEdit;
class QPushButton;
class QLabel;
class QScrollArea;

// MermaidPanel — utilitário M12. Renderiza diagramas Mermaid via subprocess
// `mmdc` (mermaid-cli). Sem `mmdc` no PATH, mostra hint de instalação.
//
// Por que mmdc + PNG: o `qt6-qtsvg-devel` não está disponível neste build,
// então não dá pra carregar SVG. PNG resolve sem essa dep e é mais
// previsível pra zoom/cópia.
class MermaidPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit MermaidPanel(QWidget* parent = nullptr);

private slots:
    void onRender();
    void onSaveAs();
    void onCopyImage();
    void onLoadExample();

private:
    bool runMmdc(const QString& src, const QString& outPath, QString* outErr) const;

    QPlainTextEdit* m_source = nullptr;
    QPushButton*    m_renderBtn = nullptr;
    QPushButton*    m_saveBtn   = nullptr;
    QPushButton*    m_copyBtn   = nullptr;
    QPushButton*    m_exampleBtn = nullptr;
    QLabel*         m_image     = nullptr;
    QScrollArea*    m_scroll    = nullptr;
    QLabel*         m_status    = nullptr;
    QString         m_lastPng;
};
