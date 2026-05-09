#pragma once

#include <QDockWidget>
#include <QPointer>

class QTextBrowser;
class QPushButton;
class QCheckBox;
class QLabel;
class QTimer;
class ScintillaEdit;

// AsciiDocPreviewPanel (M65) — preview ao vivo via asciidoctor CLI.
class AsciiDocPreviewPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit AsciiDocPreviewPanel(QWidget* parent = nullptr);
    void setActiveEditor(ScintillaEdit* ed);
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
