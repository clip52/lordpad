#include "SshConnectDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

SshConnectDialog::SshConnectDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle(tr("Conectar via SSH"));
    resize(520, 460);

    m_host = new QLineEdit(this); m_host->setPlaceholderText(tr("host.example.com"));
    m_user = new QLineEdit(this);
    m_port = new QSpinBox(this);  m_port->setRange(1, 65535); m_port->setValue(22);
    m_id   = new QLineEdit(this); m_id->setPlaceholderText(tr("(opcional) caminho da chave privada"));
    auto* btnPick = new QPushButton(tr("…"), this);
    btnPick->setMaximumWidth(40);
    m_x11  = new QCheckBox(tr("Encaminhar X11 (-X)"), this);

    auto* idRow = new QHBoxLayout();
    idRow->addWidget(m_id, 1);
    idRow->addWidget(btnPick);

    auto* form = new QFormLayout();
    form->addRow(tr("Host:"),     m_host);
    form->addRow(tr("Usuário:"),  m_user);
    form->addRow(tr("Porta:"),    m_port);
    form->addRow(tr("Identidade:"), idRow);
    form->addRow(QString(),       m_x11);

    auto* recentLabel = new QLabel(tr("Recentes:"), this);
    m_recent = new QListWidget(this);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto* lay = new QVBoxLayout(this);
    lay->addLayout(form);
    lay->addWidget(recentLabel);
    lay->addWidget(m_recent, 1);
    lay->addWidget(bb);

    connect(btnPick,  &QPushButton::clicked, this, &SshConnectDialog::onPickIdentity);
    connect(m_recent, &QListWidget::itemActivated,
            this, [this](QListWidgetItem*) { onRecentItemActivated(); });

    loadRecent();
}

QString SshConnectDialog::host() const         { return m_host->text().trimmed(); }
QString SshConnectDialog::user() const         { return m_user->text().trimmed(); }
int     SshConnectDialog::port() const         { return m_port->value(); }
QString SshConnectDialog::identityFile() const { return m_id->text().trimmed(); }
bool    SshConnectDialog::forceX11() const     { return m_x11->isChecked(); }

QStringList SshConnectDialog::sshArgs() const
{
    QStringList args;
    if (forceX11()) args << QStringLiteral("-X");
    if (port() != 22) args << QStringLiteral("-p") << QString::number(port());
    if (!identityFile().isEmpty()) args << QStringLiteral("-i") << identityFile();
    QString target;
    if (!user().isEmpty()) target = user() + QStringLiteral("@") + host();
    else                   target = host();
    args << target;
    return args;
}

void SshConnectDialog::accept()
{
    if (host().isEmpty()) {
        m_host->setFocus();
        return;
    }
    saveRecent();
    QDialog::accept();
}

void SshConnectDialog::onPickIdentity()
{
    const QString p = QFileDialog::getOpenFileName(this, tr("Chave SSH"),
        QDir::homePath() + "/.ssh");
    if (!p.isEmpty()) m_id->setText(p);
}

void SshConnectDialog::onRecentItemActivated()
{
    auto* it = m_recent->currentItem();
    if (!it) return;
    // Stored as "user@host:port" with optional " :: identity" suffix.
    QString text = it->text();
    QString idPart;
    const int sep = text.indexOf(QStringLiteral(" :: "));
    if (sep >= 0) { idPart = text.mid(sep + 4); text = text.left(sep); }
    int port = 22;
    int colon = text.lastIndexOf(':');
    if (colon > 0 && colon > text.indexOf('@')) {
        port = text.mid(colon + 1).toInt();
        text.truncate(colon);
    }
    int at = text.indexOf('@');
    if (at >= 0) {
        m_user->setText(text.left(at));
        m_host->setText(text.mid(at + 1));
    } else {
        m_host->setText(text);
    }
    m_port->setValue(port);
    m_id->setText(idPart);
}

void SshConnectDialog::loadRecent()
{
    QSettings s;
    const QStringList list = s.value(QStringLiteral("Ssh/recent")).toStringList();
    m_recent->clear();
    for (const QString& e : list) m_recent->addItem(e);
}

void SshConnectDialog::saveRecent()
{
    QString entry = (user().isEmpty() ? host() : user() + "@" + host())
                  + QStringLiteral(":") + QString::number(port());
    if (!identityFile().isEmpty()) entry += QStringLiteral(" :: ") + identityFile();

    QSettings s;
    QStringList list = s.value(QStringLiteral("Ssh/recent")).toStringList();
    list.removeAll(entry);
    list.prepend(entry);
    while (list.size() > 16) list.removeLast();
    s.setValue(QStringLiteral("Ssh/recent"), list);
}
