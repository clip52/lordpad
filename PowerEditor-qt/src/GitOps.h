#pragma once

#include <QString>
#include <QStringList>
#include <QList>

// GitOps — a thin namespace around `git` subprocess calls for the M10 polish
// features (blame, log, commit). Each function spawns a one-shot QProcess,
// runs the requested git invocation and returns parsed output. Errors come
// back as the function's bool return; the caller fills *outErr from stderr.
//
// Anything fancier (concurrency, watching, in-process libgit2) is out of
// scope. The editor only invokes these synchronously from user-driven UI.
namespace GitOps {

struct BlameLine {
    QString sha;          // 40-char hash, or "0000…" for uncommitted
    QString author;
    QString authorMail;
    QString summary;      // first line of the commit message
    QString authorDate;   // ISO-8601 author date (UTC)
};

struct LogEntry {
    QString sha;          // 7-char short sha
    QString date;         // YYYY-MM-DD
    QString author;
    QString subject;      // first line of the commit message
};

// Try to find the repo root for `filePath`. Returns empty string if the
// file isn't under a git repo.
QString repoRootFor(const QString& filePath);

// `git blame -L line,line --porcelain` for the given file at the given line
// (1-based). Returns true on success and fills `out`.
bool blameLine(const QString& filePath, int line, BlameLine& out, QString* outErr = nullptr);

// `git log --max-count=N --pretty=format:...` for the repo containing
// `filePath`. When `pathFilter` is non-empty, restricts log to that path.
bool log(const QString& filePath, int maxCount,
         const QString& pathFilter, QList<LogEntry>& out, QString* outErr = nullptr);

// `git diff <sha>~ <sha>` — full text of the diff for a commit.
bool showDiff(const QString& filePath, const QString& sha,
              QString& out, QString* outErr = nullptr);

// `git status --porcelain=v1` rows for the repo containing `filePath`.
struct StatusRow {
    QString staged;       // 'M', 'A', 'D', 'R', '?', ' '
    QString unstaged;     // same alphabet
    QString path;
};
bool status(const QString& filePath, QList<StatusRow>& out, QString* outErr = nullptr);

// Stage a file (`git add -- <path>`).
bool add(const QString& filePath, const QString& targetPath, QString* outErr = nullptr);

// Unstage a file (`git reset HEAD -- <path>`).
bool resetIndex(const QString& filePath, const QString& targetPath, QString* outErr = nullptr);

// `git commit -m "<msg>"`. Runs in the repo root that contains filePath.
bool commit(const QString& filePath, const QString& message, QString* outErr = nullptr);

// ---- M12: branches + stash ------------------------------------------------

struct Branch {
    QString name;       // "master", "origin/main", ...
    bool    isCurrent = false;
    bool    isRemote  = false;
};
bool branches(const QString& filePath, bool includeRemote,
              QList<Branch>& out, QString* outErr = nullptr);

// Returns the current branch name (or empty for detached HEAD).
QString currentBranch(const QString& filePath);

// Checkout an existing branch (`git checkout <name>`).
bool checkoutBranch(const QString& filePath, const QString& name, QString* outErr = nullptr);

// Create a new branch from `fromRef` (empty = HEAD) and checkout it.
bool createBranch(const QString& filePath, const QString& newName,
                  const QString& fromRef, QString* outErr = nullptr);

struct StashEntry {
    QString ref;        // "stash@{0}"
    QString subject;    // "WIP on master: …"
};
bool stashList(const QString& filePath, QList<StashEntry>& out, QString* outErr = nullptr);
bool stashPush(const QString& filePath, const QString& message,
               bool includeUntracked, QString* outErr = nullptr);
bool stashApply(const QString& filePath, const QString& ref, bool drop,
                QString* outErr = nullptr);
bool stashDrop(const QString& filePath, const QString& ref, QString* outErr = nullptr);

// ---- M19: remote / patch -------------------------------------------------
bool fetch(const QString& filePath, const QString& remote /*"" = origin*/,
           QString& outLog, QString* outErr = nullptr);
bool pull(const QString& filePath, QString& outLog, QString* outErr = nullptr);
bool push(const QString& filePath, const QString& remote /*"" = origin*/,
          const QString& branch /*"" = HEAD*/, QString& outLog,
          QString* outErr = nullptr);

// `git apply` of an arbitrary patch text. Returns false (and fills outErr)
// when the patch fails to apply cleanly; pass `check=true` to dry-run.
bool applyPatch(const QString& filePath, const QString& patchText,
                bool check, QString* outErr = nullptr);

} // namespace GitOps
