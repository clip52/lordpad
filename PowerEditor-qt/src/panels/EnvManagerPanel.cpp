#include "EnvManagerPanel.h"

#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>

EnvManagerPanel::EnvManagerPanel(QWidget* parent) : QDockWidget(tr(".env"), parent)
{
    setObjectName(QStringLiteral("EnvManagerPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_pathEdit = new QLineEdit(root); m_pathEdit->setFont(mono);
    m_pickBtn = new QPushButton(tr("…"), root);
    m_loadBtn = new QPushButton(tr("Carregar"), root);
    m_saveBtn = new QPushButton(tr("Salvar"), root);
    m_addBtn  = new QPushButton(tr("+ linha"), root);
    m_delBtn  = new QPushButton(tr("− linha"), root);
    m_table = new QTableWidget(root);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({ tr("Chave"), tr("Valor") });
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_status = new QLabel(root);

    auto* row1 = new QHBoxLayout();
    row1->addWidget(new QLabel(tr("Arquivo:"), root));
    row1->addWidget(m_pathEdit, 1);
    row1->addWidget(m_pickBtn);
    row1->addWidget(m_loadBtn);
    row1->addWidget(m_saveBtn);
    auto* row2 = new QHBoxLayout();
    row2->addWidget(m_addBtn);
    row2->addWidget(m_delBtn);
    row2->addStretch(1);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row1);
    lay->addLayout(row2);
    lay->addWidget(m_table, 1);
    lay->addWidget(m_status);
    setWidget(root);

    QSettings s;
    m_pathEdit->setText(s.value(QStringLiteral("Env/path")).toString());

    connect(m_pickBtn, &QPushButton::clicked, this, &EnvManagerPanel::onPickFile);
    connect(m_loadBtn, &QPushButton::clicked, this, &EnvManagerPanel::onLoad);
    connect(m_saveBtn, &QPushButton::clicked, this, &EnvManagerPanel::onSave);
    connect(m_addBtn,  &QPushButton::clicked, this, &EnvManagerPanel::onAddRow);
    connect(m_delBtn,  &QPushButton::clicked, this, &EnvManagerPanel::onDelRow);
}

void EnvManagerPanel::onPickFile()
{
    const QString p = QFileDialog::getOpenFileName(this, tr(".env"), m_pathEdit->text(),
                                                    tr("env (*.env *);;Todos (*)"));
    if (!p.isEmpty()) m_pathEdit->setText(p);
}

void EnvManagerPanel::onLoad()
{
    const QString p = m_pathEdit->text().trimmed();
    if (p.isEmpty()) return;
    QFile f(p);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_status->setText(tr("Não consegui abrir."));
        return;
    }
    QSettings().setValue(QStringLiteral("Env/path"), p);
    m_table->setRowCount(0);
    QTextStream ts(&f);
    while (!ts.atEnd()) {
        const QString line = ts.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;
        const int eq = line.indexOf('=');
        if (eq < 0) continue;
        QString key = line.left(eq).trimmed();
        QString val = line.mid(eq + 1).trimmed();
        if (val.size() >= 2 && val.startsWith('"') && val.endsWith('"'))
            val = val.mid(1, val.size() - 2);
        const int r = m_table->rowCount();
        m_table->insertRow(r);
        m_table->setItem(r, 0, new QTableWidgetItem(key));
        m_table->setItem(r, 1, new QTableWidgetItem(val));
    }
    m_status->setText(tr("%1 entradas").arg(m_table->rowCount()));
}

QString EnvManagerPanel::render() const
{
    QStringList out;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto* k = m_table->item(r, 0);
        auto* v = m_table->item(r, 1);
        if (!k || k->text().trimmed().isEmpty()) continue;
        QString val = v ? v->text() : QString();
        // quote if contains spaces or shell metas.
        if (val.contains(QRegularExpression(QStringLiteral("[\\s\"#$]"))))
            val = QStringLiteral("\"") + val.replace('"', QStringLiteral("\\\"")) + QStringLiteral("\"");
        out << k->text().trimmed() + QStringLiteral("=") + val;
    }
    return out.join('\n') + '\n';
}

void EnvManagerPanel::onSave()
{
    const QString p = m_pathEdit->text().trimmed();
    if (p.isEmpty()) return;
    QFile f(p);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_status->setText(tr("Não consegui escrever."));
        return;
    }
    f.write(render().toUtf8());
    m_status->setText(tr("Salvo."));
}

void EnvManagerPanel::onAddRow()
{
    const int r = m_table->rowCount();
    m_table->insertRow(r);
    m_table->setItem(r, 0, new QTableWidgetItem(QString()));
    m_table->setItem(r, 1, new QTableWidgetItem(QString()));
}

void EnvManagerPanel::onDelRow()
{
    const int r = m_table->currentRow();
    if (r >= 0) m_table->removeRow(r);
}
