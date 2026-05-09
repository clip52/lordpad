#pragma once

#include <QDockWidget>
#include <QStringList>

class QLineEdit;
class QPushButton;
class QTreeView;
class QFileSystemModel;
class QListWidget;
class QListWidgetItem;
class QLabel;

// SshfsPanel — monta diretórios remotos via sshfs CLI; abre arquivos
// do mountpoint como tabs normais (signal openFileRequested).
class SshfsPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit SshfsPanel(QWidget* parent = nullptr);
    ~SshfsPanel() override;
signals:
    void openFileRequested(const QString& path);
private slots:
    void onMount();
    void onUnmount();
    void onPickMountpoint();
    void onMountSelected();
    void onFileActivated(const QModelIndex& idx);
private:
    void persist() const;
    void load();
    QString defaultMountpoint(const QString& target) const;

    QLineEdit*        m_target = nullptr;     // user@host:remote/path
    QLineEdit*        m_mountpoint = nullptr; // local dir
    QPushButton*      m_pickBtn = nullptr;
    QPushButton*      m_mountBtn = nullptr;
    QPushButton*      m_umountBtn = nullptr;
    QListWidget*      m_mounts = nullptr;
    QFileSystemModel* m_fsModel = nullptr;
    QTreeView*        m_browser = nullptr;
    QLabel*           m_status = nullptr;
};
