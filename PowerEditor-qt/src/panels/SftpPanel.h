#pragma once

#include <QDockWidget>
#include <QHash>
#include <QPointer>
#include <QString>

class SftpClient;
class QLineEdit;
class QSpinBox;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QLabel;

// SftpPanel — same UX as FtpPanel, but file ops route through SftpClient
// (i.e. via the system's openssh client). Open-remote behaviour is identical:
// downloaded files land in a per-host cache folder and are re-uploaded when
// the host calls commitRemoteSave() after a successful save.
class SftpPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit SftpPanel(QWidget* parent = nullptr);
    ~SftpPanel() override;

    bool isRemoteCachedFile(const QString& localPath, QString* outRemotePath = nullptr) const;

public slots:
    void commitRemoteSave(const QString& localPath);

signals:
    void openLocalFile(const QString& localPath);
    void statusMessage(const QString& text);

private slots:
    void onConnect();
    void onDisconnect();
    void onRefresh();
    void onItemDoubleClicked(QTreeWidgetItem* item, int col);
    void onUpload();
    void onDownload();
    void onOpenRemote();
    void onDelete();
    void onCdUp();
    void onPickIdentity();

private:
    void rebuildTree();
    QString currentRemoteSelection() const;
    QString cacheRoot() const;
    QString cacheLocalFor(const QString& remoteAbsolute) const;

    SftpClient* m_client = nullptr;

    QLineEdit*   m_host = nullptr;
    QLineEdit*   m_user = nullptr;
    QSpinBox*    m_port = nullptr;
    QLineEdit*   m_id   = nullptr;
    QPushButton* m_btnId = nullptr;
    QPushButton* m_btnConnect    = nullptr;
    QPushButton* m_btnDisconnect = nullptr;

    QLabel*      m_pathLabel = nullptr;
    QTreeWidget* m_tree      = nullptr;
    QPushButton* m_btnRefresh    = nullptr;
    QPushButton* m_btnCdUp       = nullptr;
    QPushButton* m_btnUpload     = nullptr;
    QPushButton* m_btnDownload   = nullptr;
    QPushButton* m_btnOpenRemote = nullptr;
    QPushButton* m_btnDelete     = nullptr;

    QString m_currentHost;
    QHash<QString, QString> m_localToRemote;
};
