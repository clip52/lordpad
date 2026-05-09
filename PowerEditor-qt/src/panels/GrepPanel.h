#pragma once

#include <QDockWidget>
#include <QPointer>

class QLineEdit;
class QPushButton;
class QCheckBox;
class QTreeWidget;
class QLabel;
class QProcess;

// GrepPanel (M52) — wrapper de ripgrep com goto-line.
class GrepPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit GrepPanel(QWidget* parent = nullptr);
signals:
    void openFileAtLine(const QString& path, int line);
private slots:
    void onPickDir();
    void onSearch();
    void onItemActivated();
    void onProcessOutput();
    void onProcessFinished();
private:
    QLineEdit*   m_pattern = nullptr;
    QLineEdit*   m_dir     = nullptr;
    QPushButton* m_pickBtn = nullptr;
    QPushButton* m_searchBtn = nullptr;
    QCheckBox*   m_caseBox = nullptr;
    QCheckBox*   m_wordBox = nullptr;
    QCheckBox*   m_regexBox = nullptr;
    QTreeWidget* m_tree    = nullptr;
    QLabel*      m_status  = nullptr;
    QPointer<QProcess> m_proc;
    QByteArray   m_buf;
    int          m_hits = 0;
};
