#include "SftpClient.h"

#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>

SftpClient::SftpClient(QObject* parent) : QObject(parent) {}

void SftpClient::setProfile(const QString& host, const QString& user,
                            quint16 port, const QString& identityFile)
{
    m_host = host;
    m_user = user;
    m_port = port;
    m_identity = identityFile;
}

QString SftpClient::sshTarget() const {
    if (m_user.isEmpty()) return m_host;
    return m_user + QStringLiteral("@") + m_host;
}

QStringList SftpClient::commonSshArgs() const {
    QStringList a;
    // BatchMode=yes makes ssh fail-fast instead of prompting for passwords —
    // that matches our key-only auth promise and avoids hanging the UI thread.
    a << QStringLiteral("-o") << QStringLiteral("BatchMode=yes");
    a << QStringLiteral("-o") << QStringLiteral("StrictHostKeyChecking=accept-new");
    if (m_port != 22) a << QStringLiteral("-p") << QString::number(m_port);
    if (!m_identity.isEmpty()) a << QStringLiteral("-i") << m_identity;
    return a;
}

QStringList SftpClient::commonScpArgs() const {
    // scp speaks the same option flags as ssh, except port is -P (capital).
    QStringList a;
    a << QStringLiteral("-o") << QStringLiteral("BatchMode=yes");
    a << QStringLiteral("-o") << QStringLiteral("StrictHostKeyChecking=accept-new");
    if (m_port != 22) a << QStringLiteral("-P") << QString::number(m_port);
    if (!m_identity.isEmpty()) a << QStringLiteral("-i") << m_identity;
    return a;
}

QString SftpClient::resolveAgainstCwd(const QString& path) const
{
    if (path.startsWith('/')) return path;
    if (m_cwd.isEmpty() || m_cwd == "/") return "/" + path;
    if (m_cwd.endsWith('/')) return m_cwd + path;
    return m_cwd + "/" + path;
}

int SftpClient::runSsh(const QStringList& remoteArgv, QByteArray* outStdout,
                       QByteArray* outStderr, int timeoutMs) const
{
    QStringList args = commonSshArgs();
    args << sshTarget();
    args += remoteArgv;

    QProcess p;
    p.start(QStringLiteral("ssh"), args);
    if (!p.waitForStarted(2000)) return -1;
    if (!p.waitForFinished(timeoutMs)) { p.kill(); p.waitForFinished(500); return -1; }

    if (outStdout) *outStdout = p.readAllStandardOutput();
    if (outStderr) *outStderr = p.readAllStandardError();
    return p.exitStatus() == QProcess::NormalExit ? p.exitCode() : -1;
}

bool SftpClient::ping(QString* outErr)
{
    QByteArray out, err;
    const int rc = runSsh({ QStringLiteral("true") }, &out, &err, 8000);
    if (rc != 0) {
        if (outErr) *outErr = QString::fromUtf8(err.isEmpty() ? out : err);
        return false;
    }
    return true;
}

QList<FtpEntry> SftpClient::parseLsLong(const QByteArray& bytes)
{
    QList<FtpEntry> entries;
    // GNU coreutils `ls -lA` emits one entry per line, with a "total NN"
    // header for non-empty dirs. Same regex shape as FtpClient::parseUnixListLine,
    // but `ls -lA` produces entries for hidden files too.
    static const QRegularExpression rx(
        QStringLiteral(R"RX(^([\-dl])\S+\s+\d+\s+\S+\s+\S+\s+(\d+)\s+\S+\s+\S+\s+\S+\s+(.*)$)RX"));

    for (const QByteArray& rawLine : bytes.split('\n')) {
        QString line = QString::fromUtf8(rawLine).trimmed();
        if (line.isEmpty() || line.startsWith(QStringLiteral("total "))) continue;
        auto m = rx.match(line);
        if (!m.hasMatch()) continue;
        FtpEntry e;
        e.isDir = (m.captured(1) == "d");
        e.size  = m.captured(2).toLongLong();
        e.name  = m.captured(3);
        if (e.name == "." || e.name == "..") continue;
        const int arrow = e.name.indexOf(QStringLiteral(" -> "));
        if (arrow >= 0) e.name = e.name.left(arrow);
        entries.append(e);
    }
    return entries;
}

bool SftpClient::listDir(const QString& path, QList<FtpEntry>& outEntries, QString* outErr)
{
    const QString target = path.isEmpty() ? m_cwd : resolveAgainstCwd(path);
    QByteArray out, err;
    // -lA: long listing including dotfiles, excluding . and ..
    // --time-style=long-iso: keeps the date column to 3 fields so the regex
    //   above stays accurate across locales (default `ls` uses month names that
    //   differ in width by language).
    const int rc = runSsh({ QStringLiteral("ls"), QStringLiteral("-lA"),
                            QStringLiteral("--time-style=long-iso"),
                            QStringLiteral("--"), target },
                          &out, &err);
    if (rc != 0) {
        if (outErr) *outErr = QString::fromUtf8(err.isEmpty() ? out : err);
        return false;
    }
    outEntries = parseLsLong(out);
    return true;
}

bool SftpClient::retrieve(const QString& remotePath, const QString& localPath,
                          QString* outErr)
{
    const QString abs = resolveAgainstCwd(remotePath);
    QStringList args = commonScpArgs();
    args << QStringLiteral("--") << QStringLiteral("%1:%2").arg(sshTarget(), abs)
         << localPath;

    QProcess p;
    p.start(QStringLiteral("scp"), args);
    if (!p.waitForStarted(2000)) {
        if (outErr) *outErr = tr("Não foi possível iniciar scp.");
        return false;
    }
    if (!p.waitForFinished(60000)) { p.kill(); p.waitForFinished(500); return false; }
    if (p.exitCode() != 0) {
        if (outErr) {
            *outErr = QString::fromUtf8(p.readAllStandardError());
            if (outErr->isEmpty()) *outErr = QString::fromUtf8(p.readAllStandardOutput());
        }
        return false;
    }
    return true;
}

bool SftpClient::store(const QString& localPath, const QString& remotePath,
                       QString* outErr)
{
    const QString abs = resolveAgainstCwd(remotePath);
    QStringList args = commonScpArgs();
    args << QStringLiteral("--") << localPath
         << QStringLiteral("%1:%2").arg(sshTarget(), abs);

    QProcess p;
    p.start(QStringLiteral("scp"), args);
    if (!p.waitForStarted(2000)) {
        if (outErr) *outErr = tr("Não foi possível iniciar scp.");
        return false;
    }
    if (!p.waitForFinished(60000)) { p.kill(); p.waitForFinished(500); return false; }
    if (p.exitCode() != 0) {
        if (outErr) {
            *outErr = QString::fromUtf8(p.readAllStandardError());
            if (outErr->isEmpty()) *outErr = QString::fromUtf8(p.readAllStandardOutput());
        }
        return false;
    }
    return true;
}

bool SftpClient::deleteFile(const QString& remotePath, QString* outErr) {
    QByteArray out, err;
    const int rc = runSsh({ QStringLiteral("rm"), QStringLiteral("--"),
                            resolveAgainstCwd(remotePath) }, &out, &err);
    if (rc != 0) { if (outErr) *outErr = QString::fromUtf8(err.isEmpty() ? out : err); return false; }
    return true;
}
bool SftpClient::mkdir(const QString& remotePath, QString* outErr) {
    QByteArray out, err;
    const int rc = runSsh({ QStringLiteral("mkdir"), QStringLiteral("--"),
                            resolveAgainstCwd(remotePath) }, &out, &err);
    if (rc != 0) { if (outErr) *outErr = QString::fromUtf8(err.isEmpty() ? out : err); return false; }
    return true;
}
bool SftpClient::rmdir(const QString& remotePath, QString* outErr) {
    QByteArray out, err;
    const int rc = runSsh({ QStringLiteral("rmdir"), QStringLiteral("--"),
                            resolveAgainstCwd(remotePath) }, &out, &err);
    if (rc != 0) { if (outErr) *outErr = QString::fromUtf8(err.isEmpty() ? out : err); return false; }
    return true;
}
