#include "TextStatsPanel.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>
#include <QHash>

#include <ScintillaEdit.h>

TextStatsPanel::TextStatsPanel(QWidget* parent) : QDockWidget(tr("Stats"), parent)
{
    setObjectName(QStringLiteral("TextStatsPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);
    auto* root = new QWidget(this);
    m_summary = new QLabel(tr("(clique Analisar)"), root);
    m_summary->setWordWrap(true);
    m_btn = new QPushButton(tr("Analisar"), root);
    m_freq = new QTableWidget(0, 2, root);
    m_freq->setHorizontalHeaderLabels({ tr("Palavra"), tr("Ocorrências") });
    m_freq->horizontalHeader()->setStretchLastSection(true);
    m_freq->verticalHeader()->setVisible(false);

    auto* row = new QHBoxLayout();
    row->addWidget(m_btn); row->addStretch(1);
    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addWidget(m_summary);
    lay->addLayout(row);
    lay->addWidget(m_freq, 1);
    setWidget(root);
    connect(m_btn, &QPushButton::clicked, this, &TextStatsPanel::onAnalyze);
}

void TextStatsPanel::onAnalyze()
{
    if (!m_editor) return;
    QByteArray bytes = m_editor->getText(m_editor->textLength() + 1);
    QString text = QString::fromUtf8(bytes);
    int chars = text.size();
    int charsNoSpace = 0;
    for (QChar c : text) if (!c.isSpace()) ++charsNoSpace;
    int lines = text.count('\n') + 1;
    QStringList words;
    static const QRegularExpression rx(QStringLiteral("\\w+"));
    auto it = rx.globalMatch(text);
    while (it.hasNext()) words << it.next().captured().toLower();
    QHash<QString, int> freq;
    for (const QString& w : words) ++freq[w];
    QList<QPair<QString,int>> sorted;
    sorted.reserve(freq.size());
    for (auto k = freq.constBegin(); k != freq.constEnd(); ++k) sorted.append({k.key(), k.value()});
    std::sort(sorted.begin(), sorted.end(), [](auto&a, auto&b){ return a.second > b.second; });

    m_summary->setText(tr("Chars: %1   sem espaços: %2   palavras: %3   linhas: %4   únicas: %5")
                           .arg(chars).arg(charsNoSpace).arg(words.size()).arg(lines).arg(freq.size()));
    m_freq->setRowCount(0);
    const int top = qMin<int>(50, sorted.size());
    for (int i = 0; i < top; ++i) {
        const int r = m_freq->rowCount(); m_freq->insertRow(r);
        m_freq->setItem(r, 0, new QTableWidgetItem(sorted[i].first));
        m_freq->setItem(r, 1, new QTableWidgetItem(QString::number(sorted[i].second)));
    }
}
