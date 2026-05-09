#include "RssReaderPanel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>
#include <QXmlStreamReader>
#include <QDesktopServices>
#include <QUrl>

RssReaderPanel::RssReaderPanel(QWidget* parent) : QDockWidget(tr("RSS"), parent)
{
    setObjectName(QStringLiteral("RssReaderPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    m_urlEdit = new QLineEdit(root);
    m_urlEdit->setPlaceholderText(QStringLiteral("https://exemplo.com/feed.xml"));
    m_addBtn = new QPushButton(tr("+"), root);
    m_delBtn = new QPushButton(tr("−"), root);
    m_fetchBtn = new QPushButton(tr("Atualizar"), root);
    m_feeds = new QListWidget(root); m_feeds->setMaximumWidth(280);
    m_items = new QListWidget(root);
    m_body  = new QPlainTextEdit(root); m_body->setReadOnly(true);
    m_status = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(m_urlEdit, 1);
    row->addWidget(m_addBtn);
    row->addWidget(m_delBtn);
    row->addWidget(m_fetchBtn);

    auto* topSplit = new QSplitter(Qt::Horizontal, root);
    topSplit->addWidget(m_feeds); topSplit->addWidget(m_items);
    topSplit->setStretchFactor(0, 1); topSplit->setStretchFactor(1, 3);

    auto* split = new QSplitter(Qt::Vertical, root);
    split->addWidget(topSplit); split->addWidget(m_body);
    split->setStretchFactor(0, 2); split->setStretchFactor(1, 1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row);
    lay->addWidget(split, 1);
    lay->addWidget(m_status);
    setWidget(root);

    m_nam = new QNetworkAccessManager(this);

    connect(m_addBtn, &QPushButton::clicked, this, &RssReaderPanel::onAddFeed);
    connect(m_delBtn, &QPushButton::clicked, this, &RssReaderPanel::onRemoveFeed);
    connect(m_fetchBtn, &QPushButton::clicked, this, &RssReaderPanel::onFetchAll);
    connect(m_feeds, &QListWidget::itemActivated, this, &RssReaderPanel::onFeedActivated);
    connect(m_items, &QListWidget::itemActivated, this, &RssReaderPanel::onItemActivated);

    load();
}

void RssReaderPanel::onAddFeed()
{
    const QString u = m_urlEdit->text().trimmed();
    if (u.isEmpty()) return;
    auto* it = new QListWidgetItem(u, m_feeds);
    it->setData(Qt::UserRole, u);
    m_urlEdit->clear();
    persist();
}
void RssReaderPanel::onRemoveFeed()
{
    auto* it = m_feeds->currentItem();
    if (!it) return;
    delete m_feeds->takeItem(m_feeds->row(it));
    persist();
}
void RssReaderPanel::onFetchAll()
{
    m_items->clear();
    m_status->setText(tr("Buscando feeds…"));
    int pending = m_feeds->count();
    if (pending == 0) { m_status->setText(tr("Nenhum feed.")); return; }
    for (int i = 0; i < m_feeds->count(); ++i) {
        const QString url = m_feeds->item(i)->data(Qt::UserRole).toString();
        QNetworkRequest req((QUrl(url)));
        auto* rep = m_nam->get(req);
        connect(rep, &QNetworkReply::finished, this, [this, rep, url]() mutable {
            const QByteArray bytes = rep->readAll();
            rep->deleteLater();
            QXmlStreamReader xr(bytes);
            while (!xr.atEnd()) {
                xr.readNext();
                if (xr.isStartElement() && (xr.name() == QStringLiteral("item")
                                          || xr.name() == QStringLiteral("entry"))) {
                    QString title, link, body;
                    while (!(xr.isEndElement() && (xr.name() == QStringLiteral("item")
                                                || xr.name() == QStringLiteral("entry")))) {
                        xr.readNext();
                        if (xr.isStartElement()) {
                            const auto n = xr.name();
                            if (n == QStringLiteral("title"))   title = xr.readElementText();
                            else if (n == QStringLiteral("link")) {
                                if (xr.attributes().hasAttribute(QStringLiteral("href")))
                                    link = xr.attributes().value(QStringLiteral("href")).toString();
                                else link = xr.readElementText();
                            }
                            else if (n == QStringLiteral("description") || n == QStringLiteral("summary") || n == QStringLiteral("content"))
                                body = xr.readElementText();
                        }
                        if (xr.atEnd()) break;
                    }
                    if (!title.isEmpty()) {
                        auto* row = new QListWidgetItem(QStringLiteral("[%1] %2").arg(QUrl(url).host(), title), m_items);
                        row->setData(Qt::UserRole, link);
                        row->setData(Qt::UserRole + 1, body);
                    }
                }
            }
        });
    }
    m_status->setText(tr("Disparado fetch de %1 feed(s).").arg(pending));
}
void RssReaderPanel::onFeedActivated(QListWidgetItem* it)
{
    if (!it) return;
    QDesktopServices::openUrl(QUrl(it->data(Qt::UserRole).toString()));
}
void RssReaderPanel::onItemActivated(QListWidgetItem* it)
{
    if (!it) return;
    const QString body = it->data(Qt::UserRole + 1).toString();
    const QString link = it->data(Qt::UserRole).toString();
    m_body->setPlainText(QStringLiteral("%1\n\n%2").arg(link, body));
}

void RssReaderPanel::persist() const
{
    QStringList urls;
    for (int i = 0; i < m_feeds->count(); ++i)
        urls << m_feeds->item(i)->data(Qt::UserRole).toString();
    QSettings s; s.setValue(QStringLiteral("Rss/feeds"), urls);
}
void RssReaderPanel::load()
{
    QSettings s;
    for (const QString& u : s.value(QStringLiteral("Rss/feeds")).toStringList()) {
        auto* it = new QListWidgetItem(u, m_feeds);
        it->setData(Qt::UserRole, u);
    }
}
