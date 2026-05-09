#include "RegexTesterPanel.h"

#include <QCheckBox>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QSplitter>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

RegexTesterPanel::RegexTesterPanel(QWidget* parent) : QDockWidget(tr("Regex"), parent)
{
    setObjectName(QStringLiteral("RegexTesterPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_pattern = new QLineEdit(root);
    m_pattern->setPlaceholderText(tr("regex (PCRE-ish)"));
    m_pattern->setFont(mono);
    m_replace = new QLineEdit(root);
    m_replace->setPlaceholderText(tr("substituição (use \\1 \\2 …)"));
    m_replace->setFont(mono);

    m_caseInsensitive = new QCheckBox(QStringLiteral("i"), root);
    m_multiline       = new QCheckBox(QStringLiteral("m"), root);
    m_dotAll          = new QCheckBox(QStringLiteral("s"), root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("Pattern:"), root));
    row1->addWidget(m_pattern, 1);
    row1->addWidget(m_caseInsensitive);
    row1->addWidget(m_multiline);
    row1->addWidget(m_dotAll);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(new QLabel(tr("Replace:"), root));
    row2->addWidget(m_replace, 1);

    m_subject = new QPlainTextEdit(root); m_subject->setFont(mono);
    m_subject->setPlaceholderText(tr("Texto a casar"));

    m_matches = new QTableWidget(0, 3, root);
    m_matches->setHorizontalHeaderLabels({ tr("Match"), tr("Captura"), tr("Conteúdo") });
    m_matches->horizontalHeader()->setStretchLastSection(true);
    m_matches->verticalHeader()->setVisible(false);

    m_replacePreview = new QPlainTextEdit(root);
    m_replacePreview->setReadOnly(true);
    m_replacePreview->setFont(mono);

    m_status = new QLabel(root);

    auto* split = new QSplitter(Qt::Vertical, root);
    split->addWidget(m_subject);
    split->addWidget(m_matches);
    split->addWidget(m_replacePreview);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(split, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_pattern, &QLineEdit::textChanged, this, &RegexTesterPanel::onRecheck);
    connect(m_replace, &QLineEdit::textChanged, this, &RegexTesterPanel::onRecheck);
    connect(m_caseInsensitive, &QCheckBox::toggled, this, &RegexTesterPanel::onRecheck);
    connect(m_multiline,       &QCheckBox::toggled, this, &RegexTesterPanel::onRecheck);
    connect(m_dotAll,          &QCheckBox::toggled, this, &RegexTesterPanel::onRecheck);
    connect(m_subject, &QPlainTextEdit::textChanged, this, &RegexTesterPanel::onRecheck);
}

void RegexTesterPanel::onRecheck()
{
    m_matches->setRowCount(0);
    m_replacePreview->clear();
    const QString pat = m_pattern->text();
    if (pat.isEmpty()) { m_status->setText(QString()); return; }

    QRegularExpression::PatternOptions opt = QRegularExpression::NoPatternOption;
    if (m_caseInsensitive->isChecked()) opt |= QRegularExpression::CaseInsensitiveOption;
    if (m_multiline->isChecked())       opt |= QRegularExpression::MultilineOption;
    if (m_dotAll->isChecked())          opt |= QRegularExpression::DotMatchesEverythingOption;
    QRegularExpression rx(pat, opt);
    if (!rx.isValid()) { m_status->setText(tr("Erro: %1").arg(rx.errorString())); return; }

    const QString text = m_subject->toPlainText();
    int total = 0;
    auto it = rx.globalMatch(text);
    while (it.hasNext()) {
        const auto m = it.next();
        ++total;
        const int row = m_matches->rowCount();
        m_matches->insertRow(row);
        m_matches->setItem(row, 0, new QTableWidgetItem(QString::number(total)));
        m_matches->setItem(row, 1, new QTableWidgetItem(QStringLiteral("$0")));
        m_matches->setItem(row, 2, new QTableWidgetItem(m.captured(0)));
        for (int g = 1; g <= m.lastCapturedIndex(); ++g) {
            const int r2 = m_matches->rowCount();
            m_matches->insertRow(r2);
            m_matches->setItem(r2, 0, new QTableWidgetItem(QString()));
            m_matches->setItem(r2, 1, new QTableWidgetItem(QStringLiteral("$%1").arg(g)));
            m_matches->setItem(r2, 2, new QTableWidgetItem(m.captured(g)));
        }
    }

    if (!m_replace->text().isEmpty()) {
        m_replacePreview->setPlainText(text.indexOf(rx) >= 0
                                           ? QString(text).replace(rx, m_replace->text())
                                           : text);
    }
    m_status->setText(tr("%1 match(es)").arg(total));
}
