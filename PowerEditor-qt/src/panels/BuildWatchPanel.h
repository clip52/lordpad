#pragma once

#include <QDockWidget>
#include <QPointer>
#include <QStringList>

class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QCheckBox;
class QLabel;
class QProcess;
class QFileSystemWatcher;
class QTimer;

// BuildWatchPanel (M50) — roda comando de build, observa diretório, re-roda em mudança.
class BuildWatchPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit BuildWatchPanel(QWidget* parent = nullptr);
    ~BuildWatchPanel() override;
private slots:
    void onPickDir();
    void onRunOnce();
    void onToggleWatch();
    void onChanged(const QString& path);
    void onTrigger();
private:
    void startProcess();

    QLineEdit*   m_dirEdit  = nullptr;
    QLineEdit*   m_cmdEdit  = nullptr;
    QPushButton* m_pickBtn  = nullptr;
    QPushButton* m_runBtn   = nullptr;
    QPushButton* m_watchBtn = nullptr;
    QCheckBox*   m_recursiveBox = nullptr;
    QPlainTextEdit* m_log = nullptr;
    QLabel*      m_status = nullptr;

    QPointer<QProcess> m_proc;
    QFileSystemWatcher* m_watcher = nullptr;
    QTimer* m_debounce = nullptr;
    bool m_watching = false;
};
