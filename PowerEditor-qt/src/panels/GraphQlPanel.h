#pragma once

#include <QDockWidget>

class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;

// GraphQlPanel (M28) — POST a uma URL GraphQL com query + variables (JSON).
class GraphQlPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit GraphQlPanel(QWidget* parent = nullptr);

private slots:
    void onSend();
    void onReplyFinished();

private:
    QLineEdit*      m_url      = nullptr;
    QLineEdit*      m_authHeader = nullptr;
    QPlainTextEdit* m_query    = nullptr;
    QPlainTextEdit* m_vars     = nullptr;
    QPlainTextEdit* m_response = nullptr;
    QPushButton*    m_sendBtn  = nullptr;
    QLabel*         m_status   = nullptr;
    QNetworkAccessManager* m_nam = nullptr;
    QNetworkReply*         m_reply = nullptr;
};
