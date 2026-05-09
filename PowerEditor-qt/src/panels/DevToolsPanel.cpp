#include "DevToolsPanel.h"

#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QCryptographicHash>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSpinBox>
#include <QTabWidget>
#include <QUrl>
#include <QUuid>
#include <QVBoxLayout>
#include <QWidget>

namespace {
QString prettyJson(const QByteArray& bytes)
{
    QJsonParseError pe{};
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &pe);
    if (pe.error == QJsonParseError::NoError)
        return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
    return QString::fromUtf8(bytes);
}

QByteArray base64UrlPad(QByteArray s)
{
    s.replace('-', '+').replace('_', '/');
    while (s.size() % 4) s.append('=');
    return s;
}
}

DevToolsPanel::DevToolsPanel(QWidget* parent) : QDockWidget(tr("DevTools"), parent)
{
    setObjectName(QStringLiteral("DevToolsPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    m_tabs = new QTabWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    // JWT
    {
        auto* w = new QWidget(); auto* lay = new QVBoxLayout(w);
        m_jwtIn  = new QPlainTextEdit(); m_jwtIn->setFont(mono);
        m_jwtIn->setPlaceholderText(QStringLiteral("Cole o token JWT (header.payload.signature)"));
        m_jwtOut = new QPlainTextEdit(); m_jwtOut->setReadOnly(true); m_jwtOut->setFont(mono);
        auto* btn = new QPushButton(tr("Decodificar"));
        lay->addWidget(m_jwtIn, 1);
        lay->addWidget(btn);
        lay->addWidget(m_jwtOut, 2);
        connect(btn, &QPushButton::clicked, this, &DevToolsPanel::onJwtDecode);
        m_tabs->addTab(w, QStringLiteral("JWT"));
    }
    // Base64
    {
        auto* w = new QWidget(); auto* lay = new QVBoxLayout(w);
        m_b64In  = new QPlainTextEdit(); m_b64In->setFont(mono);
        m_b64Out = new QPlainTextEdit(); m_b64Out->setFont(mono);
        auto* enc = new QPushButton(tr("Encode")); auto* dec = new QPushButton(tr("Decode"));
        auto* row = new QHBoxLayout(); row->addWidget(enc); row->addWidget(dec); row->addStretch(1);
        lay->addWidget(m_b64In, 1); lay->addLayout(row); lay->addWidget(m_b64Out, 1);
        connect(enc, &QPushButton::clicked, this, &DevToolsPanel::onB64Encode);
        connect(dec, &QPushButton::clicked, this, &DevToolsPanel::onB64Decode);
        m_tabs->addTab(w, QStringLiteral("Base64"));
    }
    // URL
    {
        auto* w = new QWidget(); auto* lay = new QVBoxLayout(w);
        m_urlIn  = new QPlainTextEdit(); m_urlIn->setFont(mono);
        m_urlOut = new QPlainTextEdit(); m_urlOut->setFont(mono);
        auto* enc = new QPushButton(tr("Encode")); auto* dec = new QPushButton(tr("Decode"));
        auto* row = new QHBoxLayout(); row->addWidget(enc); row->addWidget(dec); row->addStretch(1);
        lay->addWidget(m_urlIn, 1); lay->addLayout(row); lay->addWidget(m_urlOut, 1);
        connect(enc, &QPushButton::clicked, this, &DevToolsPanel::onUrlEncode);
        connect(dec, &QPushButton::clicked, this, &DevToolsPanel::onUrlDecode);
        m_tabs->addTab(w, QStringLiteral("URL"));
    }
    // UUID
    {
        auto* w = new QWidget(); auto* lay = new QVBoxLayout(w);
        m_uuidCount = new QSpinBox(); m_uuidCount->setRange(1, 1000); m_uuidCount->setValue(5);
        auto* btn = new QPushButton(tr("Gerar UUIDs"));
        m_uuidOut = new QPlainTextEdit(); m_uuidOut->setReadOnly(true); m_uuidOut->setFont(mono);
        auto* row = new QHBoxLayout();
        row->addWidget(new QLabel(tr("Quantidade:"))); row->addWidget(m_uuidCount); row->addWidget(btn); row->addStretch(1);
        lay->addLayout(row); lay->addWidget(m_uuidOut, 1);
        connect(btn, &QPushButton::clicked, this, &DevToolsPanel::onUuidGen);
        m_tabs->addTab(w, QStringLiteral("UUID"));
    }
    // Hash
    {
        auto* w = new QWidget(); auto* lay = new QVBoxLayout(w);
        m_hashAlg = new QComboBox();
        m_hashAlg->addItems({ QStringLiteral("MD5"), QStringLiteral("SHA-1"),
                              QStringLiteral("SHA-256"), QStringLiteral("SHA-512") });
        m_hashIn  = new QPlainTextEdit(); m_hashIn->setFont(mono);
        m_hashOut = new QLineEdit(); m_hashOut->setReadOnly(true); m_hashOut->setFont(mono);
        auto* btn = new QPushButton(tr("Calcular"));
        auto* row = new QHBoxLayout();
        row->addWidget(new QLabel(tr("Algoritmo:"))); row->addWidget(m_hashAlg);
        row->addStretch(1); row->addWidget(btn);
        lay->addLayout(row); lay->addWidget(m_hashIn, 1); lay->addWidget(m_hashOut);
        connect(btn, &QPushButton::clicked, this, &DevToolsPanel::onHashCompute);
        m_tabs->addTab(w, QStringLiteral("Hash"));
    }
    // Lorem
    {
        auto* w = new QWidget(); auto* lay = new QVBoxLayout(w);
        m_loremPara = new QSpinBox(); m_loremPara->setRange(1, 30); m_loremPara->setValue(3);
        auto* btn = new QPushButton(tr("Gerar"));
        m_loremOut = new QPlainTextEdit();
        auto* row = new QHBoxLayout();
        row->addWidget(new QLabel(tr("Parágrafos:"))); row->addWidget(m_loremPara); row->addWidget(btn); row->addStretch(1);
        lay->addLayout(row); lay->addWidget(m_loremOut, 1);
        connect(btn, &QPushButton::clicked, this, &DevToolsPanel::onLoremGen);
        m_tabs->addTab(w, QStringLiteral("Lorem"));
    }
    // Password
    {
        auto* w = new QWidget(); auto* lay = new QVBoxLayout(w);
        m_pwLen   = new QSpinBox(); m_pwLen->setRange(4, 128); m_pwLen->setValue(20);
        m_pwUpper = new QCheckBox(tr("Maiúsculas")); m_pwUpper->setChecked(true);
        m_pwDigits = new QCheckBox(tr("Dígitos"));    m_pwDigits->setChecked(true);
        m_pwSymbols = new QCheckBox(tr("Símbolos"));  m_pwSymbols->setChecked(true);
        m_pwOut = new QLineEdit(); m_pwOut->setReadOnly(true); m_pwOut->setFont(mono);
        auto* btn = new QPushButton(tr("Gerar"));
        auto* copyBtn = new QPushButton(tr("Copiar"));
        auto* row1 = new QHBoxLayout();
        row1->addWidget(new QLabel(tr("Tam.:"))); row1->addWidget(m_pwLen);
        row1->addWidget(m_pwUpper); row1->addWidget(m_pwDigits); row1->addWidget(m_pwSymbols);
        row1->addStretch(1); row1->addWidget(btn);
        auto* row2 = new QHBoxLayout();
        row2->addWidget(m_pwOut, 1); row2->addWidget(copyBtn);
        lay->addLayout(row1); lay->addLayout(row2); lay->addStretch(1);
        connect(btn,     &QPushButton::clicked, this, &DevToolsPanel::onPasswordGen);
        connect(copyBtn, &QPushButton::clicked, this, [this](){ QApplication::clipboard()->setText(m_pwOut->text()); });
        m_tabs->addTab(w, QStringLiteral("Password"));
    }

    auto* root = new QWidget(this);
    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(2, 2, 2, 2);
    lay->addWidget(m_tabs);
    setWidget(root);
}

void DevToolsPanel::onJwtDecode()
{
    const QStringList parts = m_jwtIn->toPlainText().trimmed().split('.');
    if (parts.size() < 2) { m_jwtOut->setPlainText(tr("Token inválido — espera-se header.payload[.sig]")); return; }
    const QByteArray header  = QByteArray::fromBase64(base64UrlPad(parts[0].toUtf8()));
    const QByteArray payload = QByteArray::fromBase64(base64UrlPad(parts[1].toUtf8()));
    QString out;
    out += QStringLiteral("=== Header ===\n") + prettyJson(header) + QStringLiteral("\n\n");
    out += QStringLiteral("=== Payload ===\n") + prettyJson(payload);
    if (parts.size() >= 3) {
        out += QStringLiteral("\n\n=== Signature ===\n") + parts[2];
    }
    m_jwtOut->setPlainText(out);
}

void DevToolsPanel::onB64Encode()
{
    m_b64Out->setPlainText(QString::fromUtf8(m_b64In->toPlainText().toUtf8().toBase64()));
}
void DevToolsPanel::onB64Decode()
{
    m_b64Out->setPlainText(QString::fromUtf8(QByteArray::fromBase64(m_b64In->toPlainText().toUtf8())));
}
void DevToolsPanel::onUrlEncode()
{
    m_urlOut->setPlainText(QString::fromUtf8(QUrl::toPercentEncoding(m_urlIn->toPlainText())));
}
void DevToolsPanel::onUrlDecode()
{
    m_urlOut->setPlainText(QUrl::fromPercentEncoding(m_urlIn->toPlainText().toUtf8()));
}

void DevToolsPanel::onUuidGen()
{
    QStringList lines;
    for (int i = 0; i < m_uuidCount->value(); ++i)
        lines << QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_uuidOut->setPlainText(lines.join('\n'));
}

void DevToolsPanel::onHashCompute()
{
    QCryptographicHash::Algorithm a = QCryptographicHash::Sha256;
    const QString name = m_hashAlg->currentText();
    if      (name == QStringLiteral("MD5"))     a = QCryptographicHash::Md5;
    else if (name == QStringLiteral("SHA-1"))   a = QCryptographicHash::Sha1;
    else if (name == QStringLiteral("SHA-256")) a = QCryptographicHash::Sha256;
    else if (name == QStringLiteral("SHA-512")) a = QCryptographicHash::Sha512;
    QCryptographicHash h(a);
    h.addData(m_hashIn->toPlainText().toUtf8());
    m_hashOut->setText(QString::fromUtf8(h.result().toHex()));
}

void DevToolsPanel::onLoremGen()
{
    static const QStringList words = {
        "lorem","ipsum","dolor","sit","amet","consectetur","adipiscing","elit",
        "sed","do","eiusmod","tempor","incididunt","ut","labore","et","dolore",
        "magna","aliqua","enim","ad","minim","veniam","quis","nostrud",
        "exercitation","ullamco","laboris","nisi","aliquip","ex","ea","commodo",
        "consequat","duis","aute","irure","reprehenderit","voluptate","velit",
        "esse","cillum","fugiat","nulla","pariatur","excepteur","sint",
        "occaecat","cupidatat","non","proident","sunt","in","culpa","qui",
        "officia","deserunt","mollit","anim","id","est","laborum",
    };
    auto* rng = QRandomGenerator::global();
    QStringList paras;
    for (int p = 0; p < m_loremPara->value(); ++p) {
        const int sentences = 4 + rng->bounded(4);
        QStringList para;
        for (int s = 0; s < sentences; ++s) {
            const int len = 6 + rng->bounded(12);
            QStringList sent;
            for (int i = 0; i < len; ++i) sent << words[rng->bounded(words.size())];
            QString S = sent.join(' ');
            S[0] = S[0].toUpper();
            S += '.';
            para << S;
        }
        paras << para.join(' ');
    }
    m_loremOut->setPlainText(paras.join("\n\n"));
}

void DevToolsPanel::onPasswordGen()
{
    QString alphabet = QStringLiteral("abcdefghijklmnopqrstuvwxyz");
    if (m_pwUpper->isChecked())   alphabet += QStringLiteral("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    if (m_pwDigits->isChecked())  alphabet += QStringLiteral("0123456789");
    if (m_pwSymbols->isChecked()) alphabet += QStringLiteral("!@#$%^&*()-_=+[]{};:,.<>/?");
    if (alphabet.isEmpty()) { m_pwOut->setText(QString()); return; }
    auto* rng = QRandomGenerator::system();
    QString out;
    out.reserve(m_pwLen->value());
    for (int i = 0; i < m_pwLen->value(); ++i)
        out.append(alphabet[rng->bounded(alphabet.size())]);
    m_pwOut->setText(out);
}
