#pragma once

#include <QDockWidget>
#include <QString>

class QLabel;
class QPushButton;
class QScrollArea;
class QSpinBox;
class QToolButton;

// ImageViewerPanel (M17) — preview de PNG/JPG/WebP/BMP/GIF/SVG.
class ImageViewerPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit ImageViewerPanel(QWidget* parent = nullptr);

    bool openFile(const QString& path);

private slots:
    void onOpen();
    void onZoomIn();
    void onZoomOut();
    void onFit();
    void onActualSize();

private:
    void rerender();

    QString      m_path;
    QImage*      m_image = nullptr;   // original pixels
    int          m_zoomPercent = 100;
    bool         m_fitToWindow = false;

    QLabel*      m_imgLabel = nullptr;
    QScrollArea* m_scroll   = nullptr;
    QPushButton* m_openBtn  = nullptr;
    QToolButton* m_zoomInBtn = nullptr;
    QToolButton* m_zoomOutBtn = nullptr;
    QToolButton* m_fitBtn = nullptr;
    QToolButton* m_actualBtn = nullptr;
    QLabel*      m_status = nullptr;
};
