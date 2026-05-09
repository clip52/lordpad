#pragma once

#include <QDockWidget>
#include <QHash>
#include <QPointer>
#include <QString>

class FtpClient;
class QLineEdit;
class QSpinBox;
class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QProgressBar;

// FtpPanel — connects to an FTP server and presents a tree of the remote
// filesystem. Files can be downloaded, opened in the editor (with auto
// re-upload on save) and uploaded from disk.
//
// Auto re-upload binding: when the user clicks "Abrir remoto", the panel
// downloads the file into a per-session cache directory and emits
// openLocalFile() asking the host (MainWindow) to open it. It also remembers
// the (localPath → remotePath) mapping; when MainWindow tells the panel a
// known cache path was just saved (via remoteSaveCommit), the panel uploads
// the file back to the server.
class FtpPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit FtpPanel(QWidget* parent = nullptr);
    ~FtpPanel() override;

    // Returns true and fills outRemotePath when `localPath` is one of our
    // cached remote files (so MainWindow knows to call commitRemoteSave).
    bool isRemoteCachedFile(const QString& localPath, QString* outRemotePath = nullptr) const;

public slots:
    // Called by MainWindow after a save to push the file back to the server.
    void commitRemoteSave(const QString& localPath);

signals:
    // Asks the host to open `localPath` as if it were a normal local file.
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

private:
    void rebuildTree();
    QString currentRemoteSelection() const;
    QString remotePathFor(const QString& name) const;
    QString cacheRoot() const;
    QString cachePathFor(const QString& host, const QString& remote) const;

    FtpClient* m_client = nullptr;

    // Connect bar widgets
    QLineEdit*   m_host = nullptr;
    QSpinBox*    m_port = nullptr;
    QLineEdit*   m_user = nullptr;
    QLineEdit*   m_pass = nullptr;
    QPushButton* m_btnConnect = nullptr;
    QPushButton* m_btnDisconnect = nullptr;

    // Tree + actions
    QLabel*       m_pathLabel = nullptr;
    QTreeWidget*  m_tree      = nullptr;
    QPushButton*  m_btnRefresh   = nullptr;
    QPushButton*  m_btnCdUp      = nullptr;
    QPushButton*  m_btnUpload    = nullptr;
    QPushButton*  m_btnDownload  = nullptr;
    QPushButton*  m_btnOpenRemote = nullptr;
    QPushButton*  m_btnDelete    = nullptr;

    QProgressBar* m_progress = nullptr;

    QString m_currentHost;   // for cache path
    QHash<QString, QString> m_localToRemote;   // cache absolute path → remote absolute path
};
