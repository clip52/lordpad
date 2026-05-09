#pragma once

#include <QDockWidget>
#include <QImage>
#include <QList>
#include <QPoint>
#include <QString>

class QLabel;
class QScrollArea;
class QPushButton;
class QListWidget;

// ImageAnnotPanel (M32) — anota imagens com pinos numerados + label.
// Click na imagem adiciona um pino na posição clicada; lista lateral
// permite editar o texto. Save grava PNG com pinos desenhados via QPainter.
class ImageAnnotPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit ImageAnnotPanel(QWidget* parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private slots:
    void onOpen();
    void onSaveAs();
    void onDeleteSelected();
    void onClear();
    void onRowEdited();

private:
    struct Pin { QPoint pos; QString label; };
    void rerender();

    QImage      m_image;
    QList<Pin>  m_pins;
    QString     m_path;

    QLabel*       m_view = nullptr;
    QScrollArea*  m_scroll = nullptr;
    QListWidget*  m_list = nullptr;
    QPushButton*  m_openBtn = nullptr;
    QPushButton*  m_saveBtn = nullptr;
    QPushButton*  m_delBtn = nullptr;
    QPushButton*  m_clearBtn = nullptr;
    QLabel*       m_status = nullptr;
};
