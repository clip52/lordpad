#pragma once

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QSet>
#include <QString>

class ScintillaEdit;

// CoverageGutter (M16) — paints per-line coverage hits/misses in a dedicated
// Scintilla margin (margin 5). Reads either an `lcov.info` tracefile or a
// per-file `<file>.gcov`; both are common outputs of gcov/clang-cov tooling.
//
// Hits are rendered with a thin green bar, misses with a thin red bar. Lines
// with no coverage record stay blank.
class CoverageGutter : public QObject {
    Q_OBJECT
public:
    explicit CoverageGutter(QObject* parent = nullptr);

    void attach(ScintillaEdit* editor, const QString& filePath);
    void detach(ScintillaEdit* editor);

    // Load an lcov.info tracefile and apply it to every currently attached
    // editor whose path is referenced.
    bool loadLcovTracefile(const QString& path, QString* outErr = nullptr);

    // Load a `<file>.gcov` file and apply only to that source file. Returns
    // false when the .gcov is malformed.
    bool loadGcovFile(const QString& gcovPath, QString* outErr = nullptr);

    // Toggle the gutter without dropping the loaded data.
    void setEnabled(bool on);
    bool isEnabled() const { return m_enabled; }

private:
    struct Coverage {
        QSet<int> hits;     // 0-based source lines covered
        QSet<int> misses;
    };

    void applyTo(ScintillaEdit* editor, const QString& filePath);
    void clearMargin(ScintillaEdit* editor);
    void configureEditor(ScintillaEdit* editor);

    QHash<QString, Coverage> m_coverage;          // file path → hits/misses
    QHash<ScintillaEdit*, QString> m_attached;    // editor → its file path
    bool m_enabled = true;
};
