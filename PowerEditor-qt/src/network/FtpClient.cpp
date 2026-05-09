#include "FtpClient.h"

#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QTimer>

namespace {

// Block until socket receives a CRLF-terminated line or `timeoutMs` elapses.
// Returns the line *without* the CRLF, or an empty string on timeout.
QString readLine(QTcpSocket* sock, int timeoutMs)
{
    QEventLoop loop;
    QTimer timer; timer.setSingleShot(true);
    QObject::connect(sock,   &QTcpSocket::readyRead, &loop, &QEventLoop::quit);
    QObject::connect(sock,   &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout,        &loop, &QEventLoop::quit);

    while (!sock->canReadLine() && sock->state() == QAbstractSocket::ConnectedState) {
        timer.start(timeoutMs);
        loop.exec();
        if (!timer.isActive()) return {};   // timeout fired
    }
    if (!sock->canReadLine()) return {};
    QByteArray line = sock->readLine();
    while (line.endsWith('\n') || line.endsWith('\r')) line.chop(1);
    return QString::fromUtf8(line);
}

// Parse "drwxr-xr-x  2 user group  4096 Jan 02 12:34 name" → FtpEntry.
// Returns name.isEmpty() entry on failure (caller drops it).
FtpEntry parseUnixListLine(const QString& line)
{
    static const QRegularExpression rx(
        QStringLiteral(R"RX(^([\-dl])\S+\s+\d+\s+\S+\s+\S+\s+(\d+)\s+\S+\s+\S+\s+\S+\s+(.*)$)RX"));
    auto m = rx.match(line);
    FtpEntry e;
    if (!m.hasMatch()) return e;
    e.isDir = (m.captured(1) == "d");
    e.size  = m.captured(2).toLongLong();
    e.name  = m.captured(3);
    // Symlinks render as "name -> target" — keep just the source name.
    int arrow = e.name.indexOf(QStringLiteral(" -> "));
    if (arrow >= 0) e.name = e.name.left(arrow);
    return e;
}

} // namespace

FtpClient::FtpClient(QObject* parent) : QObject(parent) {}

FtpClient::~FtpClient() { disconnectFromHost(); }

bool FtpClient::isConnected() const {
    return m_ctrl && m_ctrl->state() == QAbstractSocket::ConnectedState;
}

void FtpClient::disconnectFromHost()
{
    if (!m_ctrl) return;
    if (m_ctrl->state() == QAbstractSocket::ConnectedState) {
        int code; QString txt;
        sendCommand(QStringLiteral("QUIT"), &code, &txt, 1500);
        m_ctrl->disconnectFromHost();
        if (m_ctrl->state() != QAbstractSocket::UnconnectedState)
            m_ctrl->waitForDisconnected(1000);
    }
    m_ctrl->deleteLater();
    m_ctrl = nullptr;
    m_cwd.clear();
}

bool FtpClient::connectAndLogin(const QString& host, quint16 port,
                                const QString& user, const QString& password,
                                QString* outErr)
{
    if (m_ctrl) disconnectFromHost();

    m_ctrl = new QTcpSocket(this);
    m_ctrl->connectToHost(host, port);
    if (!m_ctrl->waitForConnected(8000)) {
        if (outErr) *outErr = tr("Falha ao conectar em %1:%2 — %3")
                                  .arg(host).arg(port).arg(m_ctrl->errorString());
        m_ctrl->deleteLater(); m_ctrl = nullptr;
        return false;
    }

    int code; QString text;
    if (!readReply(&code, &text)) {
        if (outErr) *outErr = tr("Sem saudação inicial do servidor.");
        return false;
    }
    emit serverMessage(text);

    if (!sendCommand(QStringLiteral("USER %1").arg(user), &code, &text)) {
        if (outErr) *outErr = tr("USER falhou: %1").arg(text); return false;
    }
    if (code == 331) {   // password required
        if (!sendCommand(QStringLiteral("PASS %1").arg(password), &code, &text)) {
            if (outErr) *outErr = tr("PASS falhou: %1").arg(text); return false;
        }
    }
    if (code != 230) {
        if (outErr) *outErr = tr("Login recusado: %1").arg(text);
        return false;
    }

    // Default to binary so RETR/STOR don't mangle line endings.
    sendCommand(QStringLiteral("TYPE I"), &code, &text);

    // Pick up the initial PWD so the panel can show "Remote: /home/foo".
    if (sendCommand(QStringLiteral("PWD"), &code, &text) && code == 257) {
        const int q1 = text.indexOf('"');
        const int q2 = text.lastIndexOf('"');
        if (q1 >= 0 && q2 > q1) m_cwd = text.mid(q1 + 1, q2 - q1 - 1);
    }
    return true;
}

bool FtpClient::sendCommand(const QString& cmd, int* outCode, QString* outText, int timeoutMs)
{
    if (!isConnected()) return false;
    const QByteArray bytes = cmd.toUtf8() + "\r\n";
    m_ctrl->write(bytes);
    if (!m_ctrl->waitForBytesWritten(timeoutMs)) return false;
    return readReply(outCode, outText, timeoutMs);
}

bool FtpClient::readReply(int* outCode, QString* outText, int timeoutMs)
{
    if (!isConnected()) return false;
    QStringList lines;
    QString multiPrefix;   // "NNN" when in a multi-line block (e.g. "220-")

    while (true) {
        const QString line = readLine(m_ctrl, timeoutMs);
        if (line.isEmpty() && !m_ctrl->canReadLine()) return false;
        lines << line;

        if (multiPrefix.isEmpty()) {
            // First line: NNN<sp>... (final) or NNN-<text> (multi-line start)
            if (line.size() < 4) return false;
            QString code = line.left(3);
            if (line[3] == QLatin1Char(' ')) {
                if (outCode) *outCode = code.toInt();
                if (outText) *outText = lines.join('\n');
                return true;
            }
            multiPrefix = code;
        } else {
            // Continuation: ends when "NNN<sp>..." matches the saved prefix.
            if (line.startsWith(multiPrefix) && line.size() > 3 && line[3] == QLatin1Char(' ')) {
                if (outCode) *outCode = multiPrefix.toInt();
                if (outText) *outText = lines.join('\n');
                return true;
            }
        }
    }
}

bool FtpClient::cwd(const QString& path, QString* outErr)
{
    int code; QString text;
    if (!sendCommand(QStringLiteral("CWD %1").arg(path), &code, &text)) {
        if (outErr) *outErr = tr("CWD falhou: timeout");
        return false;
    }
    if (code != 250 && code != 200) {
        if (outErr) *outErr = text;
        return false;
    }
    if (sendCommand(QStringLiteral("PWD"), &code, &text) && code == 257) {
        const int q1 = text.indexOf('"');
        const int q2 = text.lastIndexOf('"');
        if (q1 >= 0 && q2 > q1) m_cwd = text.mid(q1 + 1, q2 - q1 - 1);
    }
    return true;
}

bool FtpClient::enterPassive(QString& outHost, quint16& outPort, QString* outErr)
{
    int code; QString text;
    if (!sendCommand(QStringLiteral("PASV"), &code, &text) || code != 227) {
        if (outErr) *outErr = tr("PASV falhou: %1").arg(text);
        return false;
    }
    // Server replies "227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)".
    static const QRegularExpression rx(
        QStringLiteral(R"RX(\((\d+),(\d+),(\d+),(\d+),(\d+),(\d+)\))RX"));
    auto m = rx.match(text);
    if (!m.hasMatch()) {
        if (outErr) *outErr = tr("PASV: resposta não reconhecida.");
        return false;
    }
    outHost = QStringLiteral("%1.%2.%3.%4")
                  .arg(m.captured(1), m.captured(2), m.captured(3), m.captured(4));
    outPort = static_cast<quint16>(m.captured(5).toUInt() * 256
                                 + m.captured(6).toUInt());
    return true;
}

bool FtpClient::listDir(const QString& path, QList<FtpEntry>& outEntries, QString* outErr)
{
    QString host; quint16 port;
    if (!enterPassive(host, port, outErr)) return false;

    QTcpSocket data;
    data.connectToHost(host, port);
    if (!data.waitForConnected(5000)) {
        if (outErr) *outErr = tr("Não foi possível abrir canal de dados (%1:%2)").arg(host).arg(port);
        return false;
    }

    int code; QString text;
    const QString cmd = path.isEmpty() ? QStringLiteral("LIST") : QStringLiteral("LIST %1").arg(path);
    if (!sendCommand(cmd, &code, &text) || (code != 150 && code != 125)) {
        if (outErr) *outErr = tr("LIST falhou: %1").arg(text);
        return false;
    }

    // Drain the data channel.
    QByteArray payload;
    while (data.state() == QAbstractSocket::ConnectedState) {
        data.waitForReadyRead(8000);
        if (data.bytesAvailable() == 0) break;
        payload += data.readAll();
    }
    payload += data.readAll();
    data.disconnectFromHost();
    if (data.state() != QAbstractSocket::UnconnectedState)
        data.waitForDisconnected(1000);

    // Final reply (226 Transfer complete).
    readReply(&code, &text);

    outEntries.clear();
    const QString listing = QString::fromUtf8(payload);
    for (const QString& line : listing.split('\n')) {
        QString trimmed = line;
        while (trimmed.endsWith('\r')) trimmed.chop(1);
        if (trimmed.isEmpty()) continue;
        FtpEntry e = parseUnixListLine(trimmed);
        if (e.name.isEmpty() || e.name == "." || e.name == "..") continue;
        outEntries.append(e);
    }
    return true;
}

bool FtpClient::retrieve(const QString& remotePath, const QString& localPath,
                         QString* outErr)
{
    QFile out(localPath);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (outErr) *outErr = tr("Não foi possível criar %1").arg(localPath);
        return false;
    }

    qint64 totalSize = -1;
    {
        int code; QString text;
        if (sendCommand(QStringLiteral("SIZE %1").arg(remotePath), &code, &text) && code == 213) {
            totalSize = text.section(' ', 1, 1).toLongLong();
        }
    }

    QString host; quint16 port;
    if (!enterPassive(host, port, outErr)) { out.close(); return false; }

    QTcpSocket data;
    data.connectToHost(host, port);
    if (!data.waitForConnected(5000)) {
        if (outErr) *outErr = tr("Sem canal de dados para RETR.");
        out.close(); return false;
    }

    int code; QString text;
    if (!sendCommand(QStringLiteral("RETR %1").arg(remotePath), &code, &text)
        || (code != 150 && code != 125)) {
        if (outErr) *outErr = tr("RETR falhou: %1").arg(text);
        out.close(); return false;
    }

    qint64 done = 0;
    while (data.state() == QAbstractSocket::ConnectedState) {
        if (!data.waitForReadyRead(15000)) break;
        const QByteArray chunk = data.readAll();
        if (chunk.isEmpty()) break;
        out.write(chunk);
        done += chunk.size();
        emit transferProgress(done, totalSize);
    }
    out.write(data.readAll());
    out.close();
    data.disconnectFromHost();

    readReply(&code, &text);
    if (code != 226 && code != 250) {
        if (outErr) *outErr = tr("Transferência incompleta: %1").arg(text);
        return false;
    }
    return true;
}

bool FtpClient::store(const QString& localPath, const QString& remotePath,
                      QString* outErr)
{
    QFile in(localPath);
    if (!in.open(QIODevice::ReadOnly)) {
        if (outErr) *outErr = tr("Não foi possível abrir %1").arg(localPath);
        return false;
    }
    const qint64 totalSize = in.size();

    QString host; quint16 port;
    if (!enterPassive(host, port, outErr)) { in.close(); return false; }

    QTcpSocket data;
    data.connectToHost(host, port);
    if (!data.waitForConnected(5000)) {
        if (outErr) *outErr = tr("Sem canal de dados para STOR.");
        in.close(); return false;
    }

    int code; QString text;
    if (!sendCommand(QStringLiteral("STOR %1").arg(remotePath), &code, &text)
        || (code != 150 && code != 125)) {
        if (outErr) *outErr = tr("STOR falhou: %1").arg(text);
        in.close(); return false;
    }

    qint64 done = 0;
    while (!in.atEnd()) {
        QByteArray chunk = in.read(64 * 1024);
        data.write(chunk);
        if (!data.waitForBytesWritten(15000)) break;
        done += chunk.size();
        emit transferProgress(done, totalSize);
    }
    in.close();
    data.disconnectFromHost();
    if (data.state() != QAbstractSocket::UnconnectedState)
        data.waitForDisconnected(2000);

    readReply(&code, &text);
    if (code != 226 && code != 250) {
        if (outErr) *outErr = tr("Upload incompleto: %1").arg(text);
        return false;
    }
    return true;
}

bool FtpClient::deleteFile(const QString& remotePath, QString* outErr) {
    int code; QString text;
    if (!sendCommand(QStringLiteral("DELE %1").arg(remotePath), &code, &text)
        || (code != 250 && code != 200)) {
        if (outErr) *outErr = text; return false;
    }
    return true;
}
bool FtpClient::mkdir(const QString& path, QString* outErr) {
    int code; QString text;
    if (!sendCommand(QStringLiteral("MKD %1").arg(path), &code, &text) || code != 257) {
        if (outErr) *outErr = text; return false;
    }
    return true;
}
bool FtpClient::rmdir(const QString& path, QString* outErr) {
    int code; QString text;
    if (!sendCommand(QStringLiteral("RMD %1").arg(path), &code, &text)
        || (code != 250 && code != 200)) {
        if (outErr) *outErr = text; return false;
    }
    return true;
}
