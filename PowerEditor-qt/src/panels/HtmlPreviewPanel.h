#pragma once

#include <QDockWidget>
#include <QPointer>

class QTextBrowser;
class QPushButton;
class QCheckBox;
class QLabel;
class QTimer;
class ScintillaEdit;

// HtmlPreviewPanel (M56) — preview ao vivo HTML/CSS via QTextBrowser (sem JS).
class HtmlPreviewPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit HtmlPreviewPanel(QWidget* parent = nullptr);
    void setActiveEditor(ScintillaEdit* editor);
public slots:
    void refresh();
private slots:
    void onModified();
private:
    QPointer<ScintillaEdit> m_editor;
    QTextBrowser* m_view = nullptr;
    QPushButton*  m_refreshBtn = nullptr;
    QCheckBox*    m_liveBox = nullptr;
    QLabel*       m_status = nullptr;
    QTimer*       m_debounce = nullptr;
};
