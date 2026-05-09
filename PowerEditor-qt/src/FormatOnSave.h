#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QHash>

class ScintillaEdit;

// FormatOnSave — runs an external formatter (clang-format, black, prettier,
// gofmt, rustfmt …) against the current buffer right before MainWindow::saveTab
// writes to disk. The formatter takes the buffer on stdin and emits the
// reformatted text on stdout; we replace the buffer if the exit code is 0.
//
// Defaults are seeded for common languages; users can override per-lexer in
// QSettings under "FormatOnSave/<lexerName>/{command,args}". The master
// toggle "FormatOnSave/Enabled" gates all of it.
class FormatOnSave : public QObject {
    Q_OBJECT
public:
    explicit FormatOnSave(QObject* parent = nullptr);

    // Master toggle (persisted).
    bool isEnabled() const  { return m_enabled; }
    void setEnabled(bool on);

    // Per-lexer config.
    struct Config {
        QString     command;        // "clang-format", "prettier", ...
        QStringList args;           // already split; ${file} substituted at run time
        bool        valid() const   { return !command.isEmpty(); }
    };
    Config configFor(const QString& lexerName) const;
    void   setConfig(const QString& lexerName, const Config& cfg);

    // Format the editor's contents in-place. Returns true and replaces the
    // buffer text on success. Failure leaves the buffer untouched.
    bool applyIfEnabled(ScintillaEdit* editor, const QString& lexerName,
                        const QString& filePath, QString* outErr = nullptr);

private:
    void seedDefaults();
    void load();

    bool m_enabled = false;
    QHash<QString, Config> m_configs;
};
