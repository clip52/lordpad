#pragma once

#include <QDockWidget>
#include <QJsonArray>
#include <QString>

class QPushButton;
class QLineEdit;
class QTableWidget;
class QLabel;
class QTimer;

// VaultPanel — cofre de senhas com criptografia AES-256-CBC via openssl CLI (PBKDF2).
// Master password fica apenas em memória até o panel ser destruído.
class VaultPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit VaultPanel(QWidget* parent = nullptr);
private slots:
    void onUnlock();
    void onLock();
    void onSave();
    void onAdd();
    void onRemove();
    void onCopyPassword();
    void onCopyUsername();
    void onChangeMaster();
    void onPickFile();
    void onClipboardClear();
private:
    bool unlockWithPassword(const QString& pw);
    bool encryptToFile(const QByteArray& plaintext, const QString& path,
                        const QString& pw, QString* err) const;
    bool decryptFromFile(const QString& path, const QString& pw,
                          QByteArray* out, QString* err) const;
    QByteArray serialize() const;
    bool loadFromJson(const QByteArray& bytes);
    void rebuildTable();
    void setLocked(bool locked);

    QString m_path;
    QString m_master;
    QJsonArray m_entries;     // [{name, username, password, url, notes}]
    bool m_locked = true;

    QLineEdit*    m_pathEdit = nullptr;
    QPushButton*  m_pickBtn  = nullptr;
    QPushButton*  m_unlockBtn = nullptr;
    QPushButton*  m_lockBtn   = nullptr;
    QPushButton*  m_saveBtn   = nullptr;
    QPushButton*  m_addBtn    = nullptr;
    QPushButton*  m_delBtn    = nullptr;
    QPushButton*  m_copyPwBtn = nullptr;
    QPushButton*  m_copyUserBtn = nullptr;
    QPushButton*  m_chPwBtn = nullptr;
    QTableWidget* m_table   = nullptr;
    QLabel*       m_status  = nullptr;
    QTimer*       m_clipTimer = nullptr;
    QString       m_clipPlanted;     // p/ limpar só se ainda for nosso valor
};
