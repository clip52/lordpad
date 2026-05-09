#pragma once

#include <QDockWidget>
#include <QPointer>

class QPlainTextEdit;
class QSpinBox;
class QPushButton;
class QLabel;
class ScintillaEdit;

// HexViewerPanel (M48) — visualização hex do buffer ativo (read-only).
class HexViewerPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit HexViewerPanel(QWidget* parent = nullptr);
    void setActiveEditor(ScintillaEdit* editor);
public slots:
    void refresh();
private:
    QString render(const QByteArray& bytes, int bytesPerRow) const;

    QPointer<ScintillaEdit> m_editor;
    QPlainTextEdit* m_view = nullptr;
    QSpinBox*       m_perRow = nullptr;
    QPushButton*    m_refreshBtn = nullptr;
    QLabel*         m_status = nullptr;
};
