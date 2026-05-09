#pragma once

#include <QDockWidget>

class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QLabel;
class QNetworkAccessManager;
class ScintillaEdit;

// PastebinPanel (M38) — sobe seleção/buffer pra 0x0.st (default) ou outro
// endpoint; cola a URL retornada no editor ativo via Ctrl+V do clipboard.
class PastebinPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit PastebinPanel(QWidget* parent = nullptr);
    void setActiveEditor(ScintillaEdit* ed) { m_editor = ed; }
private slots:
    void onUpload();
private:
    ScintillaEdit*  m_editor = nullptr;
    QLineEdit*      m_endpoint = nullptr;
    QPlainTextEdit* m_log = nullptr;
    QPushButton*    m_btn = nullptr;
    QLabel*         m_status = nullptr;
    QNetworkAccessManager* m_nam = nullptr;
};
