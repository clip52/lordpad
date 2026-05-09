#pragma once

#include <QDockWidget>
#include <QString>
#include <QStringList>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QLabel;
class QNetworkAccessManager;
class QNetworkReply;

// RestClientPanel (M15) — bare-bones HTTP client for the editor.
//
// UI: method combo (GET/POST/PUT/DELETE/PATCH/HEAD/OPTIONS), URL line edit,
// headers table (key/value), body editor, send button. Response area shows
// status line, headers, and body (with naive JSON pretty-print when the
// response Content-Type advertises JSON).
//
// Persistence: last URL + headers in QSettings under "RestClient/last/*",
// so reopening the panel doesn't force the user to retype.
class RestClientPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit RestClientPanel(QWidget* parent = nullptr);

private slots:
    void onSend();
    void onAddHeader();
    void onRemoveHeader();
    void onReplyFinished();

private:
    void persistLast() const;
    void loadLast();

    QComboBox*      m_method = nullptr;
    QLineEdit*      m_url    = nullptr;
    QTableWidget*   m_headers = nullptr;
    QPlainTextEdit* m_body   = nullptr;
    QPlainTextEdit* m_response = nullptr;
    QPushButton*    m_sendBtn = nullptr;
    QPushButton*    m_addHdrBtn = nullptr;
    QPushButton*    m_delHdrBtn = nullptr;
    QLabel*         m_status = nullptr;

    QNetworkAccessManager* m_nam = nullptr;
    QNetworkReply*         m_reply = nullptr;
};
