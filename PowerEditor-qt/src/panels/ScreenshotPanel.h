#pragma once

#include <QDockWidget>
#include <QString>

class QLabel;
class QPushButton;
class QSpinBox;
class QComboBox;

// ScreenshotPanel (M45) — captura tela via subprocess (`gnome-screenshot`,
// `spectacle`, `scrot` ou `import`) e exibe / salva o PNG.
class ScreenshotPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit ScreenshotPanel(QWidget* parent = nullptr);
private slots:
    void onCapture();
    void onSaveAs();
    void onCopy();
private:
    QString availableTool() const;
    QComboBox*   m_mode = nullptr;     // tela inteira / janela / região
    QSpinBox*    m_delay = nullptr;
    QPushButton* m_captureBtn = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_copyBtn = nullptr;
    QLabel*      m_thumb = nullptr;
    QLabel*      m_status = nullptr;
    QString      m_tmpPath;
};
