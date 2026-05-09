#pragma once

#include <QDockWidget>

class QLineEdit;
class QPushButton;
class QPlainTextEdit;
class QLabel;

// SslCertViewerPanel (M64) — exibe info de cert .pem/.crt via openssl x509.
class SslCertViewerPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit SslCertViewerPanel(QWidget* parent = nullptr);
private slots:
    void onPickFile();
    void onLoad();
    void onLoadHost();
private:
    QLineEdit*      m_pathEdit = nullptr;
    QLineEdit*      m_hostEdit = nullptr;
    QPushButton*    m_pickBtn = nullptr;
    QPushButton*    m_loadBtn = nullptr;
    QPushButton*    m_hostBtn = nullptr;
    QPlainTextEdit* m_out = nullptr;
    QLabel*         m_status = nullptr;
};
