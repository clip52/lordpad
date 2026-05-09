#pragma once

#include <QDialog>
#include <QString>
#include <QStringList>

class QLineEdit;
class QSpinBox;
class QCheckBox;
class QListWidget;

// Small dialog for opening an SSH session: host / user / port / optional
// identity file. Recently used "user@host:port" entries are persisted in
// QSettings under "Ssh/recent" so the user doesn't retype them.
class SshConnectDialog : public QDialog {
    Q_OBJECT
public:
    explicit SshConnectDialog(QWidget* parent = nullptr);

    // Filled after the user clicks OK.
    QString host() const;
    QString user() const;
    int     port() const;
    QString identityFile() const;
    bool    forceX11() const;

    // Convenience: assemble the argv for `ssh` based on the inputs.
    QStringList sshArgs() const;

private slots:
    void onPickIdentity();
    void onRecentItemActivated();
    void accept() override;

private:
    void loadRecent();
    void saveRecent();

    QLineEdit* m_host  = nullptr;
    QLineEdit* m_user  = nullptr;
    QSpinBox*  m_port  = nullptr;
    QLineEdit* m_id    = nullptr;
    QCheckBox* m_x11   = nullptr;
    QListWidget* m_recent = nullptr;
};
