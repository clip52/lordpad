#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>

#include "FtpClient.h"   // reuses FtpEntry — semantically the same row shape

// SftpClient — SFTP-equivalent file operations implemented on top of the
// system's openssh client. Each op is a one-shot subprocess (`ssh ... ls`,
// `scp`, etc.); for editor-style usage that's fine, and it sidesteps the
// libssh-devel dependency entirely.
//
// Auth: key-based only. The user can pass an identity file via setProfile;
// otherwise ssh-agent / ~/.ssh/id_* are used. Password prompts are NOT
// surfaced — if the server demands one, the subprocess will block until
// a TTY appears and the call will time out.
//
// Performance note: each call performs a fresh SSH handshake unless the
// user has ControlMaster/ControlPersist configured in ~/.ssh/config. We
// don't try to manage that — it's a per-host concern and openssh handles
// it transparently when set.
class SftpClient : public QObject {
    Q_OBJECT
public:
    explicit SftpClient(QObject* parent = nullptr);

    void setProfile(const QString& host, const QString& user,
                    quint16 port, const QString& identityFile);

    // Quick reachability check: runs `ssh ... true`. Used by the panel's
    // "Connect" button to surface an error early.
    bool ping(QString* outErr = nullptr);

    bool listDir(const QString& path, QList<FtpEntry>& outEntries,
                 QString* outErr = nullptr);
    bool retrieve(const QString& remotePath, const QString& localPath,
                  QString* outErr = nullptr);
    bool store(const QString& localPath, const QString& remotePath,
               QString* outErr = nullptr);
    bool deleteFile(const QString& remotePath, QString* outErr = nullptr);
    bool mkdir(const QString& remotePath, QString* outErr = nullptr);
    bool rmdir(const QString& remotePath, QString* outErr = nullptr);

    // Virtual cwd — kept entirely on the client side (SSH is stateless).
    QString currentDir() const { return m_cwd; }
    void    setCurrentDir(const QString& d) { m_cwd = d; }
    QString resolveAgainstCwd(const QString& path) const;

    // The "user@host" target string passed to ssh / scp.
    QString sshTarget() const;
    QStringList commonSshArgs() const;
    QStringList commonScpArgs() const;

signals:
    void transferProgress(qint64 bytesDone, qint64 bytesTotal);

private:
    // Run an ssh-side command, capture combined stdout+stderr. Returns the
    // process exit code (-1 when the process couldn't be started / timed out).
    int runSsh(const QStringList& remoteArgv, QByteArray* outStdout,
               QByteArray* outStderr, int timeoutMs = 15000) const;

    static QList<FtpEntry> parseLsLong(const QByteArray& bytes);

    QString m_host;
    QString m_user;
    quint16 m_port = 22;
    QString m_identity;
    QString m_cwd = QStringLiteral("/");
};
