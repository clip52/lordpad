#include "GitOps.h"

#include <QDateTime>
#include <QFileInfo>
#include <QProcess>
#include <QTimeZone>

namespace GitOps {

namespace {

// Run git with `args` in `cwd`. Returns exit code (-1 on failure to start /
// timeout). Captures stdout into *out and stderr into *err (when non-null).
int runGit(const QString& cwd, const QStringList& args,
           QByteArray* out = nullptr, QByteArray* err = nullptr,
           int timeoutMs = 15000)
{
    QProcess p;
    p.setWorkingDirectory(cwd);
    p.start(QStringLiteral("git"), args);
    if (!p.waitForStarted(2000)) return -1;
    if (!p.waitForFinished(timeoutMs)) { p.kill(); p.waitForFinished(500); return -1; }
    if (out) *out = p.readAllStandardOutput();
    if (err) *err = p.readAllStandardError();
    return p.exitStatus() == QProcess::NormalExit ? p.exitCode() : -1;
}

} // namespace

QString repoRootFor(const QString& filePath)
{
    if (filePath.isEmpty()) return {};
    const QString dir = QFileInfo(filePath).absolutePath();
    QByteArray out;
    if (runGit(dir, { QStringLiteral("rev-parse"), QStringLiteral("--show-toplevel") },
               &out) != 0) return {};
    return QString::fromUtf8(out).trimmed();
}

bool blameLine(const QString& filePath, int line, BlameLine& out, QString* outErr)
{
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) {
        if (outErr) *outErr = QObject::tr("Arquivo não está em um repositório git.");
        return false;
    }
    // git accepts an absolute path as long as it's inside the repo.
    QStringList args = {
        QStringLiteral("blame"),
        QStringLiteral("-L"), QStringLiteral("%1,%1").arg(line),
        QStringLiteral("--porcelain"),
        QStringLiteral("--"), filePath
    };
    QByteArray bytesOut, bytesErr;
    if (runGit(root, args, &bytesOut, &bytesErr) != 0) {
        if (outErr) *outErr = QString::fromUtf8(bytesErr).trimmed();
        return false;
    }

    // Porcelain shape:
    //   <sha> <orig> <final> <count>
    //   author <name>
    //   author-mail <<addr>>
    //   author-time <unix>
    //   ... (committer fields)
    //   summary <text>
    //   <TAB><line content>
    out = {};
    for (const QByteArray& rawLine : bytesOut.split('\n')) {
        const QString l = QString::fromUtf8(rawLine);
        if (out.sha.isEmpty() && !l.isEmpty() && l.size() >= 40
            && l[0].isLetterOrNumber()) {
            out.sha = l.left(40);
            continue;
        }
        if (l.startsWith(QLatin1String("author ")))      out.author      = l.mid(7);
        else if (l.startsWith(QLatin1String("author-mail "))) out.authorMail  = l.mid(12);
        else if (l.startsWith(QLatin1String("summary ")))     out.summary     = l.mid(8);
        else if (l.startsWith(QLatin1String("author-time "))) {
            // Convert unix epoch to ISO-8601 (UTC) — keep it short.
            const auto secs = l.mid(12).trimmed().toLongLong();
            out.authorDate = QDateTime::fromSecsSinceEpoch(secs, QTimeZone::UTC)
                                 .toString(Qt::ISODate);
        }
    }
    return !out.sha.isEmpty();
}

bool log(const QString& filePath, int maxCount,
         const QString& pathFilter, QList<LogEntry>& out, QString* outErr)
{
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) {
        if (outErr) *outErr = QObject::tr("Arquivo não está em um repositório git.");
        return false;
    }
    QStringList args = {
        QStringLiteral("log"),
        QStringLiteral("--max-count=%1").arg(maxCount),
        QStringLiteral("--date=short"),
        // Use a unique field separator so we can split safely even when
        // commit messages contain tabs or pipes.
        QStringLiteral("--pretty=format:%h\x1F%ad\x1F%an\x1F%s")
    };
    if (!pathFilter.isEmpty()) {
        args << QStringLiteral("--") << pathFilter;
    }

    QByteArray bytesOut, bytesErr;
    if (runGit(root, args, &bytesOut, &bytesErr) != 0) {
        if (outErr) *outErr = QString::fromUtf8(bytesErr).trimmed();
        return false;
    }
    out.clear();
    for (const QByteArray& rawLine : bytesOut.split('\n')) {
        if (rawLine.isEmpty()) continue;
        const QString l = QString::fromUtf8(rawLine);
        const QStringList cols = l.split(QChar(0x1F));
        if (cols.size() < 4) continue;
        LogEntry e;
        e.sha     = cols[0];
        e.date    = cols[1];
        e.author  = cols[2];
        e.subject = cols[3];
        out.append(e);
    }
    return true;
}

bool showDiff(const QString& filePath, const QString& sha, QString& out, QString* outErr)
{
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) {
        if (outErr) *outErr = QObject::tr("Arquivo não está em um repositório git.");
        return false;
    }
    QByteArray bytesOut, bytesErr;
    if (runGit(root, { QStringLiteral("show"),
                       QStringLiteral("--no-color"),
                       QStringLiteral("--patch"),
                       sha },
               &bytesOut, &bytesErr) != 0) {
        if (outErr) *outErr = QString::fromUtf8(bytesErr).trimmed();
        return false;
    }
    out = QString::fromUtf8(bytesOut);
    return true;
}

bool status(const QString& filePath, QList<StatusRow>& out, QString* outErr)
{
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) {
        if (outErr) *outErr = QObject::tr("Arquivo não está em um repositório git.");
        return false;
    }
    QByteArray bytesOut, bytesErr;
    if (runGit(root, { QStringLiteral("status"),
                       QStringLiteral("--porcelain=v1") },
               &bytesOut, &bytesErr) != 0) {
        if (outErr) *outErr = QString::fromUtf8(bytesErr).trimmed();
        return false;
    }
    out.clear();
    for (const QByteArray& rawLine : bytesOut.split('\n')) {
        if (rawLine.size() < 4) continue;
        StatusRow r;
        r.staged   = QString::fromUtf8(rawLine.mid(0, 1));
        r.unstaged = QString::fromUtf8(rawLine.mid(1, 1));
        r.path     = QString::fromUtf8(rawLine.mid(3));
        // `git status --porcelain` for renamed entries uses "from -> to";
        // keep the destination path for display.
        const int arrow = r.path.indexOf(QStringLiteral(" -> "));
        if (arrow >= 0) r.path = r.path.mid(arrow + 4);
        out.append(r);
    }
    return true;
}

bool add(const QString& filePath, const QString& targetPath, QString* outErr) {
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) { if (outErr) *outErr = QObject::tr("Sem repositório."); return false; }
    QByteArray err;
    if (runGit(root, { QStringLiteral("add"), QStringLiteral("--"), targetPath }, nullptr, &err) != 0) {
        if (outErr) *outErr = QString::fromUtf8(err).trimmed(); return false;
    }
    return true;
}
bool resetIndex(const QString& filePath, const QString& targetPath, QString* outErr) {
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) { if (outErr) *outErr = QObject::tr("Sem repositório."); return false; }
    QByteArray err;
    if (runGit(root, { QStringLiteral("reset"), QStringLiteral("HEAD"),
                       QStringLiteral("--"), targetPath }, nullptr, &err) != 0) {
        if (outErr) *outErr = QString::fromUtf8(err).trimmed(); return false;
    }
    return true;
}
// ---------------------------------------------------------------------------
// M12: branches + stash
// ---------------------------------------------------------------------------
bool branches(const QString& filePath, bool includeRemote,
              QList<Branch>& out, QString* outErr)
{
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) { if (outErr) *outErr = QObject::tr("Sem repositório."); return false; }
    QStringList args = {
        QStringLiteral("branch"),
        QStringLiteral("--list"),
        QStringLiteral("--format=%(HEAD)\x1F%(refname:short)\x1F%(upstream:short)"),
    };
    if (includeRemote) args << QStringLiteral("--all");

    QByteArray bytesOut, bytesErr;
    if (runGit(root, args, &bytesOut, &bytesErr) != 0) {
        if (outErr) *outErr = QString::fromUtf8(bytesErr).trimmed();
        return false;
    }
    out.clear();
    for (const QByteArray& rawLine : bytesOut.split('\n')) {
        if (rawLine.isEmpty()) continue;
        const QString line = QString::fromUtf8(rawLine);
        const QStringList cols = line.split(QChar(0x1F));
        if (cols.size() < 2) continue;
        Branch b;
        b.isCurrent = (cols[0] == QStringLiteral("*"));
        b.name      = cols[1];
        // Remote branches show up as "origin/foo" with `--all`. We mark them
        // by prefix match; keeps the dialog filter lightweight.
        b.isRemote  = b.name.startsWith(QStringLiteral("remotes/"))
                   || b.name.contains(QStringLiteral("/"));
        if (b.name.startsWith(QStringLiteral("remotes/")))
            b.name = b.name.mid(8);   // strip "remotes/"
        out.append(b);
    }
    return true;
}

QString currentBranch(const QString& filePath)
{
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) return {};
    QByteArray out;
    if (runGit(root, { QStringLiteral("rev-parse"),
                       QStringLiteral("--abbrev-ref"),
                       QStringLiteral("HEAD") }, &out) != 0) return {};
    QString s = QString::fromUtf8(out).trimmed();
    if (s == QStringLiteral("HEAD")) return {};   // detached
    return s;
}

bool checkoutBranch(const QString& filePath, const QString& name, QString* outErr)
{
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) { if (outErr) *outErr = QObject::tr("Sem repositório."); return false; }
    QByteArray err;
    if (runGit(root, { QStringLiteral("checkout"), QStringLiteral("--"), name },
               nullptr, &err) != 0) {
        // Fall back to plain checkout — `--` after `checkout` confuses git when
        // `name` is a branch (it disambiguates as path). Retry without it.
        err.clear();
        if (runGit(root, { QStringLiteral("checkout"), name }, nullptr, &err) != 0) {
            if (outErr) *outErr = QString::fromUtf8(err).trimmed(); return false;
        }
    }
    return true;
}

bool createBranch(const QString& filePath, const QString& newName,
                  const QString& fromRef, QString* outErr)
{
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) { if (outErr) *outErr = QObject::tr("Sem repositório."); return false; }
    QStringList args = { QStringLiteral("checkout"), QStringLiteral("-b"), newName };
    if (!fromRef.isEmpty()) args << fromRef;
    QByteArray err;
    if (runGit(root, args, nullptr, &err) != 0) {
        if (outErr) *outErr = QString::fromUtf8(err).trimmed();
        return false;
    }
    return true;
}

bool stashList(const QString& filePath, QList<StashEntry>& out, QString* outErr)
{
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) { if (outErr) *outErr = QObject::tr("Sem repositório."); return false; }
    QByteArray bytesOut, bytesErr;
    if (runGit(root, { QStringLiteral("stash"), QStringLiteral("list"),
                       QStringLiteral("--format=%gd\x1F%s") },
               &bytesOut, &bytesErr) != 0) {
        if (outErr) *outErr = QString::fromUtf8(bytesErr).trimmed();
        return false;
    }
    out.clear();
    for (const QByteArray& rawLine : bytesOut.split('\n')) {
        if (rawLine.isEmpty()) continue;
        const QString line = QString::fromUtf8(rawLine);
        const QStringList cols = line.split(QChar(0x1F));
        if (cols.size() < 2) continue;
        StashEntry e;
        e.ref     = cols[0];
        e.subject = cols[1];
        out.append(e);
    }
    return true;
}

bool stashPush(const QString& filePath, const QString& message,
               bool includeUntracked, QString* outErr)
{
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) { if (outErr) *outErr = QObject::tr("Sem repositório."); return false; }
    QStringList args = { QStringLiteral("stash"), QStringLiteral("push") };
    if (includeUntracked) args << QStringLiteral("-u");
    if (!message.isEmpty()) args << QStringLiteral("-m") << message;
    QByteArray err;
    if (runGit(root, args, nullptr, &err) != 0) {
        if (outErr) *outErr = QString::fromUtf8(err).trimmed();
        return false;
    }
    return true;
}

bool stashApply(const QString& filePath, const QString& ref, bool drop, QString* outErr)
{
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) { if (outErr) *outErr = QObject::tr("Sem repositório."); return false; }
    QByteArray err;
    const QString verb = drop ? QStringLiteral("pop") : QStringLiteral("apply");
    if (runGit(root, { QStringLiteral("stash"), verb, ref }, nullptr, &err) != 0) {
        if (outErr) *outErr = QString::fromUtf8(err).trimmed();
        return false;
    }
    return true;
}

bool stashDrop(const QString& filePath, const QString& ref, QString* outErr)
{
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) { if (outErr) *outErr = QObject::tr("Sem repositório."); return false; }
    QByteArray err;
    if (runGit(root, { QStringLiteral("stash"), QStringLiteral("drop"), ref }, nullptr, &err) != 0) {
        if (outErr) *outErr = QString::fromUtf8(err).trimmed();
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// M19: remote + patch
// ---------------------------------------------------------------------------
bool fetch(const QString& filePath, const QString& remote, QString& outLog, QString* outErr)
{
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) { if (outErr) *outErr = QObject::tr("Sem repositório."); return false; }
    QStringList args = { QStringLiteral("fetch") };
    if (!remote.isEmpty()) args << remote; else args << QStringLiteral("--all");
    args << QStringLiteral("--prune") << QStringLiteral("--tags");
    QByteArray out, err;
    const int rc = runGit(root, args, &out, &err, 60000);
    outLog = QString::fromUtf8(out) + QString::fromUtf8(err);
    if (rc != 0) { if (outErr) *outErr = QString::fromUtf8(err).trimmed(); return false; }
    return true;
}
bool pull(const QString& filePath, QString& outLog, QString* outErr)
{
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) { if (outErr) *outErr = QObject::tr("Sem repositório."); return false; }
    QByteArray out, err;
    const int rc = runGit(root, { QStringLiteral("pull"), QStringLiteral("--ff-only") },
                          &out, &err, 60000);
    outLog = QString::fromUtf8(out) + QString::fromUtf8(err);
    if (rc != 0) { if (outErr) *outErr = QString::fromUtf8(err).trimmed(); return false; }
    return true;
}
bool push(const QString& filePath, const QString& remote, const QString& branch,
          QString& outLog, QString* outErr)
{
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) { if (outErr) *outErr = QObject::tr("Sem repositório."); return false; }
    QStringList args = { QStringLiteral("push") };
    if (!remote.isEmpty()) args << remote;
    if (!branch.isEmpty()) args << branch;
    QByteArray out, err;
    const int rc = runGit(root, args, &out, &err, 60000);
    outLog = QString::fromUtf8(out) + QString::fromUtf8(err);
    if (rc != 0) { if (outErr) *outErr = QString::fromUtf8(err).trimmed(); return false; }
    return true;
}
bool applyPatch(const QString& filePath, const QString& patchText, bool check, QString* outErr)
{
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) { if (outErr) *outErr = QObject::tr("Sem repositório."); return false; }
    QStringList args = { QStringLiteral("apply") };
    if (check) args << QStringLiteral("--check");
    args << QStringLiteral("-");
    QProcess p;
    p.setWorkingDirectory(root);
    p.start(QStringLiteral("git"), args);
    if (!p.waitForStarted(2000)) {
        if (outErr) *outErr = QObject::tr("Falha ao iniciar git apply."); return false;
    }
    p.write(patchText.toUtf8());
    p.closeWriteChannel();
    if (!p.waitForFinished(15000)) { p.kill(); p.waitForFinished(500);
        if (outErr) *outErr = QObject::tr("git apply demorou demais."); return false;
    }
    if (p.exitCode() != 0) {
        if (outErr) *outErr = QString::fromUtf8(p.readAllStandardError()).trimmed();
        return false;
    }
    return true;
}

bool commit(const QString& filePath, const QString& message, QString* outErr) {
    const QString root = repoRootFor(filePath);
    if (root.isEmpty()) { if (outErr) *outErr = QObject::tr("Sem repositório."); return false; }
    QByteArray err;
    if (runGit(root, { QStringLiteral("commit"), QStringLiteral("-m"), message },
               nullptr, &err) != 0) {
        if (outErr) *outErr = QString::fromUtf8(err).trimmed(); return false;
    }
    return true;
}

} // namespace GitOps
