#pragma once

#include <QDockWidget>
#include <QString>

class QPushButton;
class QTreeWidget;
class QTreeWidgetItem;
class QPlainTextEdit;
class QLabel;

// ArchiveBrowserPanel (M25) — listagem read-only de zip/tar via subprocess.
// Browse: `unzip -l` / `tar -tvf`. Extract single: `unzip -p` / `tar -xOf`.
class ArchiveBrowserPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit ArchiveBrowserPanel(QWidget* parent = nullptr);

private slots:
    void onOpen();
    void onItemActivated(QTreeWidgetItem* it, int col);

private:
    enum class Kind { None, Zip, Tar };
    Kind detectKind(const QString& path) const;
    void rebuild();

    QString          m_path;
    Kind             m_kind = Kind::None;
    QPushButton*     m_openBtn = nullptr;
    QTreeWidget*     m_tree = nullptr;
    QPlainTextEdit*  m_preview = nullptr;
    QLabel*          m_status = nullptr;
};
