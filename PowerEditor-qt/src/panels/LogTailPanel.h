#pragma once

#include <QDockWidget>
#include <QHash>
#include <QPointer>

class QLineEdit;
class QPushButton;
class QListWidget;
class QPlainTextEdit;
class QLabel;
class QProcess;

// LogTailPanel (M70) — multi-arquivo tail -f com timestamps por linha.
class LogTailPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit LogTailPanel(QWidget* parent = nullptr);
    ~LogTailPanel() override;
private slots:
    void onPickFile();
    void onAdd();
    void onRemove();
    void onClear();
private:
    void persist() const;
    void load();
    void startTailFor(const QString& path);

    QLineEdit*      m_pathEdit = nullptr;
    QPushButton*    m_pickBtn = nullptr;
    QPushButton*    m_addBtn = nullptr;
    QPushButton*    m_delBtn = nullptr;
    QPushButton*    m_clearBtn = nullptr;
    QListWidget*    m_files = nullptr;
    QPlainTextEdit* m_out = nullptr;
    QLabel*         m_status = nullptr;
    QHash<QString, QProcess*> m_procs;
};
