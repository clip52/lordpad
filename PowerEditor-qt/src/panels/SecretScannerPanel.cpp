#include "SecretScannerPanel.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include <ScintillaEdit.h>

namespace {
struct Pattern { const char* label; const char* regex; };
const Pattern kPatterns[] = {
    { "AWS Access Key",      R"(AKIA[0-9A-Z]{16})" },
    { "AWS Secret Key",      R"((?:[A-Za-z0-9/+]{40})(?=[^A-Za-z0-9/+]|$))" },
    { "GitHub PAT (classic)",R"(ghp_[A-Za-z0-9]{36,})" },
    { "GitHub PAT (fine)",   R"(github_pat_[A-Za-z0-9_]{82})" },
    { "Slack Token",         R"(xox[abprs]-[A-Za-z0-9-]{10,})" },
    { "Google API key",      R"(AIza[0-9A-Za-z\-_]{35})" },
    { "Stripe live",         R"(sk_live_[0-9a-zA-Z]{24,})" },
    { "JWT",                 R"(eyJ[A-Za-z0-9_-]{8,}\.eyJ[A-Za-z0-9_-]{8,}\.[A-Za-z0-9_-]{8,})" },
    { "RSA Private Key",     R"(-----BEGIN (?:RSA |EC |OPENSSH |DSA )?PRIVATE KEY-----)" },
    { "Bearer / Authorization", R"((?:authorization|bearer)\s*[:=]\s*['\"][^'\"\n]{16,}['\"])" },
};
}

SecretScannerPanel::SecretScannerPanel(QWidget* parent) : QDockWidget(tr("Secrets"), parent)
{
    setObjectName(QStringLiteral("SecretScannerPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);
    auto* root = new QWidget(this);
    m_scanBtn = new QPushButton(tr("Varrer buffer"), root);
    m_findings = new QTreeWidget(root);
    m_findings->setHeaderLabels({ tr("Tipo"), tr("Linha"), tr("Trecho") });
    m_findings->setRootIsDecorated(false);
    m_findings->header()->setStretchLastSection(true);
    m_status = new QLabel(root);

    auto* row = new QHBoxLayout(); row->addWidget(m_scanBtn); row->addStretch(1);
    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row);
    lay->addWidget(m_findings, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_scanBtn, &QPushButton::clicked, this, &SecretScannerPanel::onScan);
    connect(m_findings, &QTreeWidget::itemActivated, this, &SecretScannerPanel::onItemActivated);
}

void SecretScannerPanel::onScan()
{
    m_findings->clear();
    if (!m_editor) { m_status->setText(tr("Sem editor.")); return; }
    QString text = QString::fromUtf8(m_editor->getText(m_editor->textLength() + 1));
    int hits = 0;
    for (const Pattern& p : kPatterns) {
        QRegularExpression rx(QString::fromLatin1(p.regex), QRegularExpression::CaseInsensitiveOption);
        if (!rx.isValid()) continue;
        auto it = rx.globalMatch(text);
        while (it.hasNext()) {
            const auto m = it.next();
            ++hits;
            const int pos = m.capturedStart();
            const int line = text.left(pos).count('\n') + 1;
            QString trecho = m.captured();
            if (trecho.size() > 80) trecho = trecho.left(80) + QStringLiteral("…");
            auto* row = new QTreeWidgetItem(m_findings);
            row->setText(0, QString::fromUtf8(p.label));
            row->setText(1, QString::number(line));
            row->setText(2, trecho);
            row->setData(0, Qt::UserRole, line);
        }
    }
    m_status->setText(hits == 0 ? tr("✓ Nenhum secret aparente.") : tr("✗ %1 achado(s)").arg(hits));
}

void SecretScannerPanel::onItemActivated(QTreeWidgetItem* it, int)
{
    if (!it) return;
    emit gotoLineRequested(it->data(0, Qt::UserRole).toInt());
}
