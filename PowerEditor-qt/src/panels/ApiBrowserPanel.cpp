#include "ApiBrowserPanel.h"

#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSplitter>
#include <QStandardPaths>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

ApiBrowserPanel::ApiBrowserPanel(QWidget* parent) : QDockWidget(tr("OpenAPI"), parent)
{
    setObjectName(QStringLiteral("ApiBrowserPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_pathEdit = new QLineEdit(root);
    m_pathEdit->setPlaceholderText(tr("openapi.json | openapi.yaml | url"));
    m_pickBtn = new QPushButton(tr("…"), root);
    m_loadBtn = new QPushButton(tr("Carregar"), root);
    m_tree = new QTreeWidget(root);
    m_tree->setHeaderLabels({ tr("Path"), tr("Métodos") });
    m_tree->setRootIsDecorated(true);
    m_detail = new QPlainTextEdit(root); m_detail->setReadOnly(true); m_detail->setFont(mono);
    m_status = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(m_pathEdit, 1);
    row->addWidget(m_pickBtn);
    row->addWidget(m_loadBtn);

    auto* split = new QSplitter(Qt::Horizontal, root);
    split->addWidget(m_tree); split->addWidget(m_detail);
    split->setStretchFactor(0, 1); split->setStretchFactor(1, 1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row);
    lay->addWidget(split, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_pickBtn, &QPushButton::clicked, this, &ApiBrowserPanel::onPickFile);
    connect(m_loadBtn, &QPushButton::clicked, this, &ApiBrowserPanel::onLoad);
    connect(m_tree, &QTreeWidget::itemActivated, this, &ApiBrowserPanel::onItemActivated);
}

void ApiBrowserPanel::onPickFile()
{
    const QString p = QFileDialog::getOpenFileName(this, tr("OpenAPI"), m_pathEdit->text(),
                                                    tr("Spec (*.json *.yaml *.yml);;Todos (*)"));
    if (!p.isEmpty()) m_pathEdit->setText(p);
}

void ApiBrowserPanel::onLoad()
{
    const QString p = m_pathEdit->text().trimmed();
    if (p.isEmpty()) return;
    QByteArray bytes;
    if (p.startsWith(QStringLiteral("http"))) {
        const QString curl = QStandardPaths::findExecutable(QStringLiteral("curl"));
        if (curl.isEmpty()) { m_status->setText(tr("curl ausente.")); return; }
        QProcess pr;
        pr.start(curl, { QStringLiteral("-fsSL"), p });
        if (!pr.waitForFinished(15000)) { pr.kill(); m_status->setText(tr("Timeout.")); return; }
        bytes = pr.readAllStandardOutput();
    } else {
        QFile f(p);
        if (!f.open(QIODevice::ReadOnly)) { m_status->setText(tr("Não consegui abrir.")); return; }
        bytes = f.readAll();
    }

    // Convert YAML→JSON via python -c yaml/json (se necessário).
    bool isYaml = !p.endsWith(QStringLiteral(".json")) && !bytes.trimmed().startsWith('{');
    if (isYaml) {
        const QString py = QStandardPaths::findExecutable(QStringLiteral("python3"));
        if (py.isEmpty()) { m_status->setText(tr("python3 ausente p/ converter YAML.")); return; }
        QProcess pp;
        pp.setProcessChannelMode(QProcess::SeparateChannels);
        pp.start(py, { QStringLiteral("-c"),
            QStringLiteral("import sys, yaml, json; json.dump(yaml.safe_load(sys.stdin), sys.stdout)") });
        pp.write(bytes); pp.closeWriteChannel();
        if (!pp.waitForFinished(10000)) { pp.kill(); m_status->setText(tr("Timeout YAML.")); return; }
        if (pp.exitCode() != 0) { m_status->setText(tr("Falha YAML.")); return; }
        bytes = pp.readAllStandardOutput();
    }

    QJsonParseError pe;
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &pe);
    if (pe.error != QJsonParseError::NoError) {
        m_status->setText(tr("JSON inválido: %1").arg(pe.errorString())); return;
    }
    const QJsonObject root = doc.object();
    const QJsonObject paths = root.value(QStringLiteral("paths")).toObject();

    m_tree->clear();
    for (auto pIt = paths.begin(); pIt != paths.end(); ++pIt) {
        auto* parent = new QTreeWidgetItem(m_tree);
        parent->setText(0, pIt.key());
        const QJsonObject ops = pIt.value().toObject();
        QStringList methods;
        for (auto opIt = ops.begin(); opIt != ops.end(); ++opIt) {
            const QString m = opIt.key().toUpper();
            if (m != QStringLiteral("PARAMETERS") && m.size() <= 8) methods << m;
            auto* child = new QTreeWidgetItem(parent);
            child->setText(0, m);
            const QJsonObject op = opIt.value().toObject();
            child->setText(1, op.value(QStringLiteral("summary")).toString());
            child->setData(0, Qt::UserRole, QJsonDocument(op).toJson(QJsonDocument::Indented));
        }
        parent->setText(1, methods.join(", "));
    }
    m_tree->expandAll();
    m_status->setText(tr("OpenAPI %1 — %2 paths")
                          .arg(root.value(QStringLiteral("openapi")).toString(
                                root.value(QStringLiteral("swagger")).toString()))
                          .arg(paths.size()));
}

void ApiBrowserPanel::onItemActivated(QTreeWidgetItem* it)
{
    if (!it) return;
    const QString json = it->data(0, Qt::UserRole).toString();
    if (!json.isEmpty()) m_detail->setPlainText(json);
}
