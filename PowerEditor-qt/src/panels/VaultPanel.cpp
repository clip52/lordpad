#include "VaultPanel.h"

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QStandardPaths>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

namespace {
constexpr int kClipboardClearMs = 30 * 1000;
}

VaultPanel::VaultPanel(QWidget* parent) : QDockWidget(tr("Cofre"), parent)
{
    setObjectName(QStringLiteral("VaultPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_pathEdit  = new QLineEdit(root); m_pathEdit->setFont(mono);
    m_pickBtn   = new QPushButton(tr("…"), root);
    m_unlockBtn = new QPushButton(tr("Destrancar"), root);
    m_lockBtn   = new QPushButton(tr("Trancar"), root);
    m_saveBtn   = new QPushButton(tr("Salvar"), root);
    m_addBtn    = new QPushButton(tr("+ entrada"), root);
    m_delBtn    = new QPushButton(tr("− entrada"), root);
    m_copyPwBtn = new QPushButton(tr("Copiar senha"), root);
    m_copyUserBtn = new QPushButton(tr("Copiar usuário"), root);
    m_chPwBtn   = new QPushButton(tr("Trocar master…"), root);

    m_table = new QTableWidget(root);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({ tr("Nome"), tr("Usuário"),
                                          tr("Senha"), tr("URL"), tr("Notas") });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_status = new QLabel(root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("Arquivo:"), root));
    row1->addWidget(m_pathEdit, 1);
    row1->addWidget(m_pickBtn);
    row1->addWidget(m_unlockBtn);
    row1->addWidget(m_lockBtn);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(m_addBtn);
    row2->addWidget(m_delBtn);
    row2->addWidget(m_copyUserBtn);
    row2->addWidget(m_copyPwBtn);
    row2->addStretch(1);
    row2->addWidget(m_chPwBtn);
    row2->addWidget(m_saveBtn);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(m_table, 1);
    lay->addWidget(m_status);
    setWidget(root);

    QSettings s;
    QString path = s.value(QStringLiteral("Vault/path")).toString();
    if (path.isEmpty())
        path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/vault.enc");
    m_pathEdit->setText(path);

    m_clipTimer = new QTimer(this);
    m_clipTimer->setSingleShot(true);
    connect(m_clipTimer, &QTimer::timeout, this, &VaultPanel::onClipboardClear);

    connect(m_pickBtn,   &QPushButton::clicked, this, &VaultPanel::onPickFile);
    connect(m_unlockBtn, &QPushButton::clicked, this, &VaultPanel::onUnlock);
    connect(m_lockBtn,   &QPushButton::clicked, this, &VaultPanel::onLock);
    connect(m_saveBtn,   &QPushButton::clicked, this, &VaultPanel::onSave);
    connect(m_addBtn,    &QPushButton::clicked, this, &VaultPanel::onAdd);
    connect(m_delBtn,    &QPushButton::clicked, this, &VaultPanel::onRemove);
    connect(m_copyPwBtn, &QPushButton::clicked, this, &VaultPanel::onCopyPassword);
    connect(m_copyUserBtn, &QPushButton::clicked, this, &VaultPanel::onCopyUsername);
    connect(m_chPwBtn,   &QPushButton::clicked, this, &VaultPanel::onChangeMaster);

    setLocked(true);
}

void VaultPanel::setLocked(bool locked)
{
    m_locked = locked;
    m_unlockBtn->setEnabled(locked);
    m_lockBtn->setEnabled(!locked);
    m_saveBtn->setEnabled(!locked);
    m_addBtn->setEnabled(!locked);
    m_delBtn->setEnabled(!locked);
    m_copyPwBtn->setEnabled(!locked);
    m_copyUserBtn->setEnabled(!locked);
    m_chPwBtn->setEnabled(!locked);
    if (locked) {
        m_master.clear();
        m_entries = QJsonArray();
        m_table->setRowCount(0);
    }
}

void VaultPanel::onPickFile()
{
    const QString p = QFileDialog::getSaveFileName(this, tr("Cofre"), m_pathEdit->text(),
                                                    tr("Cofre (*.enc);;Todos (*)"),
                                                    nullptr, QFileDialog::DontConfirmOverwrite);
    if (!p.isEmpty()) m_pathEdit->setText(p);
}

bool VaultPanel::encryptToFile(const QByteArray& plaintext, const QString& path,
                                 const QString& pw, QString* err) const
{
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("openssl"));
    if (tool.isEmpty()) { if (err) *err = tr("openssl não encontrado."); return false; }
    QProcess p;
    p.setProcessChannelMode(QProcess::SeparateChannels);
    p.start(tool, { QStringLiteral("enc"), QStringLiteral("-aes-256-cbc"),
                     QStringLiteral("-salt"), QStringLiteral("-pbkdf2"),
                     QStringLiteral("-iter"), QStringLiteral("100000"),
                     QStringLiteral("-pass"), QStringLiteral("stdin") });
    if (!p.waitForStarted(3000)) { if (err) *err = tr("falha ao iniciar openssl"); return false; }
    p.write(pw.toUtf8() + '\n');
    p.write(plaintext);
    p.closeWriteChannel();
    if (!p.waitForFinished(15000)) { p.kill(); if (err) *err = tr("timeout"); return false; }
    if (p.exitCode() != 0) { if (err) *err = QString::fromUtf8(p.readAllStandardError()); return false; }
    const QByteArray out = p.readAllStandardOutput();
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (err) *err = f.errorString();
        return false;
    }
    f.write(out);
    return true;
}

bool VaultPanel::decryptFromFile(const QString& path, const QString& pw,
                                   QByteArray* out, QString* err) const
{
    const QString tool = QStandardPaths::findExecutable(QStringLiteral("openssl"));
    if (tool.isEmpty()) { if (err) *err = tr("openssl não encontrado."); return false; }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) { if (err) *err = f.errorString(); return false; }
    const QByteArray bytes = f.readAll();

    QProcess p;
    p.setProcessChannelMode(QProcess::SeparateChannels);
    p.start(tool, { QStringLiteral("enc"), QStringLiteral("-d"),
                     QStringLiteral("-aes-256-cbc"),
                     QStringLiteral("-pbkdf2"),
                     QStringLiteral("-iter"), QStringLiteral("100000"),
                     QStringLiteral("-pass"), QStringLiteral("stdin") });
    if (!p.waitForStarted(3000)) { if (err) *err = tr("falha ao iniciar openssl"); return false; }
    p.write(pw.toUtf8() + '\n');
    p.write(bytes);
    p.closeWriteChannel();
    if (!p.waitForFinished(15000)) { p.kill(); if (err) *err = tr("timeout"); return false; }
    if (p.exitCode() != 0) {
        if (err) *err = tr("senha incorreta ou arquivo corrompido");
        return false;
    }
    if (out) *out = p.readAllStandardOutput();
    return true;
}

QByteArray VaultPanel::serialize() const
{
    return QJsonDocument(m_entries).toJson(QJsonDocument::Compact);
}

bool VaultPanel::loadFromJson(const QByteArray& bytes)
{
    QJsonParseError pe;
    const auto doc = QJsonDocument::fromJson(bytes, &pe);
    if (pe.error != QJsonParseError::NoError) return false;
    if (!doc.isArray()) return false;
    m_entries = doc.array();
    rebuildTable();
    return true;
}

void VaultPanel::rebuildTable()
{
    m_table->setRowCount(m_entries.size());
    for (int i = 0; i < m_entries.size(); ++i) {
        const auto obj = m_entries[i].toObject();
        m_table->setItem(i, 0, new QTableWidgetItem(obj.value(QStringLiteral("name")).toString()));
        m_table->setItem(i, 1, new QTableWidgetItem(obj.value(QStringLiteral("username")).toString()));
        auto* pwItem = new QTableWidgetItem(QString(obj.value(QStringLiteral("password")).toString().size(), QChar(0x2022)));
        pwItem->setData(Qt::UserRole, obj.value(QStringLiteral("password")).toString());
        m_table->setItem(i, 2, pwItem);
        m_table->setItem(i, 3, new QTableWidgetItem(obj.value(QStringLiteral("url")).toString()));
        m_table->setItem(i, 4, new QTableWidgetItem(obj.value(QStringLiteral("notes")).toString()));
    }
}

bool VaultPanel::unlockWithPassword(const QString& pw)
{
    const QString path = m_pathEdit->text().trimmed();
    if (path.isEmpty()) return false;

    if (!QFile::exists(path)) {
        // first-time creation: empty vault.
        m_master = pw;
        m_entries = QJsonArray();
        rebuildTable();
        QSettings().setValue(QStringLiteral("Vault/path"), path);
        m_status->setText(tr("Cofre novo (sem entries) — adicione e salve."));
        return true;
    }
    QByteArray plain; QString err;
    if (!decryptFromFile(path, pw, &plain, &err)) {
        m_status->setText(err.isEmpty() ? tr("Falha ao destrancar.") : err);
        return false;
    }
    if (!loadFromJson(plain)) {
        m_status->setText(tr("Conteúdo descriptografado é inválido."));
        return false;
    }
    m_master = pw;
    QSettings().setValue(QStringLiteral("Vault/path"), path);
    m_status->setText(tr("Destrancado — %1 entradas").arg(m_entries.size()));
    return true;
}

void VaultPanel::onUnlock()
{
    bool ok = false;
    const QString pw = QInputDialog::getText(this, tr("Master password"),
                                                tr("Senha mestre:"), QLineEdit::Password,
                                                QString(), &ok);
    if (!ok || pw.isEmpty()) return;
    if (unlockWithPassword(pw)) setLocked(false);
}

void VaultPanel::onLock()
{
    setLocked(true);
    m_status->setText(tr("Trancado."));
}

void VaultPanel::onSave()
{
    if (m_locked) return;
    // sync edited cells back into m_entries before writing.
    for (int r = 0; r < m_table->rowCount(); ++r) {
        QJsonObject obj;
        obj[QStringLiteral("name")]     = m_table->item(r, 0) ? m_table->item(r, 0)->text() : QString();
        obj[QStringLiteral("username")] = m_table->item(r, 1) ? m_table->item(r, 1)->text() : QString();
        // Password stays in UserRole; user may have re-pasted via "edit raw" dialog (later).
        obj[QStringLiteral("password")] = m_table->item(r, 2) ? m_table->item(r, 2)->data(Qt::UserRole).toString() : QString();
        obj[QStringLiteral("url")]      = m_table->item(r, 3) ? m_table->item(r, 3)->text() : QString();
        obj[QStringLiteral("notes")]    = m_table->item(r, 4) ? m_table->item(r, 4)->text() : QString();
        if (r < m_entries.size()) m_entries[r] = obj;
        else                       m_entries.append(obj);
    }
    while (m_entries.size() > m_table->rowCount()) m_entries.removeLast();

    const QString path = m_pathEdit->text().trimmed();
    QDir().mkpath(QFileInfo(path).absolutePath());
    QString err;
    if (!encryptToFile(serialize(), path, m_master, &err)) {
        QMessageBox::warning(this, tr("Vault"), tr("Falha ao salvar: %1").arg(err));
        return;
    }
    m_status->setText(tr("Salvo."));
}

void VaultPanel::onAdd()
{
    const int r = m_table->rowCount();
    m_table->insertRow(r);
    m_table->setItem(r, 0, new QTableWidgetItem(tr("nova entrada")));
    m_table->setItem(r, 1, new QTableWidgetItem(QString()));
    auto* pwItem = new QTableWidgetItem(QString());
    pwItem->setData(Qt::UserRole, QString());
    m_table->setItem(r, 2, pwItem);
    m_table->setItem(r, 3, new QTableWidgetItem(QString()));
    m_table->setItem(r, 4, new QTableWidgetItem(QString()));
}

void VaultPanel::onRemove()
{
    const int r = m_table->currentRow();
    if (r < 0) return;
    m_table->removeRow(r);
}

void VaultPanel::onCopyPassword()
{
    const int r = m_table->currentRow();
    if (r < 0) return;
    auto* it = m_table->item(r, 2);
    if (!it) return;
    QString pw = it->data(Qt::UserRole).toString();
    if (pw.isEmpty()) {
        bool ok = false;
        pw = QInputDialog::getText(this, tr("Senha"), tr("Definir senha:"), QLineEdit::Password,
                                     QString(), &ok);
        if (!ok || pw.isEmpty()) return;
        it->setData(Qt::UserRole, pw);
        it->setText(QString(pw.size(), QChar(0x2022)));
    }
    QApplication::clipboard()->setText(pw);
    m_clipPlanted = pw;
    m_clipTimer->start(kClipboardClearMs);
    m_status->setText(tr("Senha copiada — clipboard limpo em %1s").arg(kClipboardClearMs / 1000));
}

void VaultPanel::onCopyUsername()
{
    const int r = m_table->currentRow();
    if (r < 0) return;
    auto* it = m_table->item(r, 1);
    if (!it) return;
    QApplication::clipboard()->setText(it->text());
    m_status->setText(tr("Usuário copiado."));
}

void VaultPanel::onChangeMaster()
{
    if (m_locked) { m_status->setText(tr("Destranque primeiro.")); return; }
    bool ok = false;
    const QString pw = QInputDialog::getText(this, tr("Trocar master"),
                                                tr("Nova senha mestre:"), QLineEdit::Password,
                                                QString(), &ok);
    if (!ok || pw.isEmpty()) return;
    const QString pw2 = QInputDialog::getText(this, tr("Trocar master"),
                                                 tr("Confirmar:"), QLineEdit::Password,
                                                 QString(), &ok);
    if (!ok || pw != pw2) {
        QMessageBox::warning(this, tr("Vault"), tr("Senhas não coincidem."));
        return;
    }
    m_master = pw;
    onSave();
    m_status->setText(tr("Master trocada e cofre re-cifrado."));
}

void VaultPanel::onClipboardClear()
{
    auto* cb = QApplication::clipboard();
    if (cb->text() == m_clipPlanted) cb->clear();
    m_clipPlanted.clear();
    m_status->setText(tr("Clipboard limpo."));
}
