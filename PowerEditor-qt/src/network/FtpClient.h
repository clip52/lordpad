#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>

class QTcpSocket;

// Subset of an FTP server's LIST output as we need it: name + size + flags
// to distinguish files from directories. Times / permissions are dropped —
// the panel doesn't display them yet.
struct FtpEntry {
    QString name;
    qint64  size = 0;
    bool    isDir = false;
};

// FtpClient — minimal RFC 959 client for browsing / upload / download.
//
// Wire shape: a single TCP control channel speaks line-oriented commands;
// every transfer (LIST, RETR, STOR) uses an ad-hoc data channel opened in
// passive (PASV) mode. The class exposes blocking-style methods that pump
// a local QEventLoop until each reply arrives, returning success bools.
// Active mode and TLS are out of scope.
class FtpClient : public QObject {
    Q_OBJECT
public:
    explicit FtpClient(QObject* parent = nullptr);
    ~FtpClient() override;

    // Connect + log in. Returns true on success; on failure outErr is filled.
    bool connectAndLogin(const QString& host, quint16 port,
                         const QString& user, const QString& password,
                         QString* outErr = nullptr);
    void disconnectFromHost();
    bool isConnected() const;

    QString currentDir() const { return m_cwd; }

    // Change the remote working directory.
    bool cwd(const QString& path, QString* outErr = nullptr);

    // List entries of `path` (or the current dir when empty). Parses the
    // common Unix-style LIST output; falls back to NLST when LIST fails.
    bool listDir(const QString& path, QList<FtpEntry>& outEntries, QString* outErr = nullptr);

    // Download a remote file into `localPath`. Always uses binary mode.
    bool retrieve(const QString& remotePath, const QString& localPath,
                  QString* outErr = nullptr);

    // Upload `localPath` to `remotePath`. Always binary.
    bool store(const QString& localPath, const QString& remotePath,
               QString* outErr = nullptr);

    // Delete a regular file.
    bool deleteFile(const QString& remotePath, QString* outErr = nullptr);

    // Make / remove a directory.
    bool mkdir(const QString& path, QString* outErr = nullptr);
    bool rmdir(const QString& path, QString* outErr = nullptr);

signals:
    // Coarse-grained progress signal so the UI can update (bytes / bytesTotal).
    // bytesTotal may be -1 when the server didn't advertise SIZE.
    void transferProgress(qint64 bytesDone, qint64 bytesTotal);

    // Server greeting / response broadcast for the panel's status line.
    void serverMessage(const QString& text);

private:
    // Send `cmd` (CRLF appended) and read until a final reply line arrives.
    // outCode/outText will hold the parsed reply.
    bool sendCommand(const QString& cmd, int* outCode, QString* outText,
                     int timeoutMs = 8000);

    // Wait for a complete reply (multi-line aware). Returns false on timeout.
    bool readReply(int* outCode, QString* outText, int timeoutMs = 8000);

    // PASV handshake — returns the data-channel host/port the server picked,
    // and the SIZE-of-file when the caller asked for it (RETR pre-flight).
    bool enterPassive(QString& outHost, quint16& outPort, QString* outErr);

    QTcpSocket* m_ctrl = nullptr;
    QString     m_cwd;
};
