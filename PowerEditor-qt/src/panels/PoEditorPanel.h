#pragma once

#include <QDockWidget>
#include <QString>
#include <QStringList>
#include <QList>

class QTableWidget;
class QPlainTextEdit;
class QPushButton;
class QLabel;

// PoEditorPanel (M16) — gettext .po file editor.
//
// .po format we recognize:
//   #-prefix comment lines (translator/extracted/reference/flag)
//   msgctxt "..."
//   msgid   "..."  ["..."]*
//   msgstr  "..."  ["..."]*
//
// Multi-line strings (continuation strings) are merged on read and emitted
// as a single string on write — we don't try to preserve column-by-column
// the original wrapping, but we do preserve comments and entry order.
class PoEditorPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit PoEditorPanel(QWidget* parent = nullptr);

    bool openFile(const QString& path);
    bool save();

private slots:
    void onOpen();
    void onSave();
    void onSelectionChanged();
    void onMsgstrChanged();

private:
    struct Entry {
        QStringList comments;   // raw "#..." lines
        QString     ctx;        // msgctxt or empty
        QString     msgid;
        QString     msgstr;
    };

    void rebuildTable();
    bool parsePoLines(const QStringList& lines);
    QString unquote(const QString& quoted) const;
    QString quote(const QString& raw) const;

    QString          m_filePath;
    QStringList      m_header;     // file-level header (the first untranslated entry)
    QList<Entry>     m_entries;
    int              m_currentRow = -1;

    QTableWidget*    m_table = nullptr;
    QPlainTextEdit*  m_msgid = nullptr;
    QPlainTextEdit*  m_msgstr = nullptr;
    QPushButton*     m_openBtn = nullptr;
    QPushButton*     m_saveBtn = nullptr;
    QLabel*          m_status = nullptr;

    bool m_suppressMsgstrSignal = false;
};
