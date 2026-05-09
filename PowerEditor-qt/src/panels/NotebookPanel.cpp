#include "NotebookPanel.h"

#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <QWidget>

NotebookPanel::NotebookPanel(QWidget* parent) : QDockWidget(tr("Notebook"), parent)
{
    setObjectName(QStringLiteral("NotebookPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);

    // Top toolbar
    m_openBtn   = new QPushButton(tr("Abrir…"), root);
    m_saveBtn   = new QPushButton(tr("Salvar"), root);
    m_addCodeBtn= new QPushButton(tr("+ Código"), root);
    m_addMdBtn  = new QPushButton(tr("+ Markdown"), root);
    m_delBtn    = new QPushButton(tr("Apagar"), root);
    m_upBtn     = new QPushButton(QStringLiteral("↑"), root);
    m_downBtn   = new QPushButton(QStringLiteral("↓"), root);
    m_runBtn    = new QPushButton(tr("Rodar célula"), root);

    auto* topRow = new QHBoxLayout();
    topRow->addWidget(m_openBtn);
    topRow->addWidget(m_saveBtn);
    topRow->addStretch(1);
    topRow->addWidget(m_addCodeBtn);
    topRow->addWidget(m_addMdBtn);
    topRow->addWidget(m_delBtn);
    topRow->addWidget(m_upBtn);
    topRow->addWidget(m_downBtn);
    topRow->addWidget(m_runBtn);

    // Left: list of cells
    m_list = new QListWidget(root);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);

    // Right: type combo + source + output
    auto* rightWrap = new QWidget(root);
    auto* rightLay  = new QVBoxLayout(rightWrap);
    rightLay->setContentsMargins(0, 0, 0, 0);
    m_typeCombo = new QComboBox(rightWrap);
    m_typeCombo->addItems({ QStringLiteral("code"), QStringLiteral("markdown") });
    m_source = new QPlainTextEdit(rightWrap);
    m_output = new QPlainTextEdit(rightWrap);
    m_output->setReadOnly(true);
    m_output->setMaximumBlockCount(2000);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    mono.setPointSize(10);
    m_source->setFont(mono);
    m_output->setFont(mono);

    auto* split = new QSplitter(Qt::Vertical, rightWrap);
    split->addWidget(m_source);
    split->addWidget(m_output);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 1);

    auto* typeRow = new QHBoxLayout();
    typeRow->addWidget(new QLabel(tr("Tipo:"), rightWrap));
    typeRow->addWidget(m_typeCombo);
    typeRow->addStretch(1);

    rightLay->addLayout(typeRow);
    rightLay->addWidget(split, 1);

    auto* hSplit = new QSplitter(Qt::Horizontal, root);
    hSplit->addWidget(m_list);
    hSplit->addWidget(rightWrap);
    hSplit->setStretchFactor(0, 1);
    hSplit->setStretchFactor(1, 3);

    m_status = new QLabel(root);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(2, 2, 2, 2);
    lay->addLayout(topRow);
    lay->addWidget(hSplit, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_list,   &QListWidget::currentRowChanged, this, [this](int){ onCellSelectionChanged(); });
    connect(m_source, &QPlainTextEdit::textChanged,    this, &NotebookPanel::onSourceChanged);
    connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int){
        if (currentRow() < 0) return;
        m_cells[currentRow()].type = m_typeCombo->currentText();
        rebuildList();
    });
    connect(m_addCodeBtn, &QPushButton::clicked, this, &NotebookPanel::onAddCode);
    connect(m_addMdBtn,   &QPushButton::clicked, this, &NotebookPanel::onAddMarkdown);
    connect(m_delBtn,     &QPushButton::clicked, this, &NotebookPanel::onDeleteCell);
    connect(m_upBtn,      &QPushButton::clicked, this, &NotebookPanel::onMoveUp);
    connect(m_downBtn,    &QPushButton::clicked, this, &NotebookPanel::onMoveDown);
    connect(m_runBtn,     &QPushButton::clicked, this, &NotebookPanel::onRunCell);
    connect(m_openBtn,    &QPushButton::clicked, this, &NotebookPanel::onOpen);
    connect(m_saveBtn,    &QPushButton::clicked, this, &NotebookPanel::onSave);

    // Seed empty notebook with a code cell so the panel isn't empty on first open.
    m_cells.append({ QStringLiteral("code"), QString(), QString() });
    rebuildList();
    m_list->setCurrentRow(0);
}

QString NotebookPanel::cellHeadline(const Cell& c)
{
    QString first = c.source.section('\n', 0, 0).trimmed();
    if (first.size() > 60) first = first.left(60) + QStringLiteral("…");
    return QStringLiteral("[%1] %2").arg(c.type.left(2), first.isEmpty() ? QStringLiteral("(vazia)") : first);
}

QListWidgetItem* NotebookPanel::itemForRow(int row) const
{
    if (row < 0 || row >= m_list->count()) return nullptr;
    return m_list->item(row);
}
int NotebookPanel::currentRow() const { return m_list->currentRow(); }

void NotebookPanel::rebuildList()
{
    const int prev = m_list->currentRow();
    m_list->clear();
    for (const Cell& c : m_cells) {
        new QListWidgetItem(cellHeadline(c), m_list);
    }
    if (prev >= 0 && prev < m_list->count()) m_list->setCurrentRow(prev);
}

void NotebookPanel::persistCurrentSource()
{
    const int row = currentRow();
    if (row < 0 || row >= m_cells.size()) return;
    m_cells[row].source = m_source->toPlainText();
}

void NotebookPanel::onCellSelectionChanged()
{
    const int row = currentRow();
    if (row < 0 || row >= m_cells.size()) {
        m_source->clear();
        m_output->clear();
        return;
    }
    m_suppressSourceSignal = true;
    m_typeCombo->setCurrentText(m_cells[row].type);
    m_source->setPlainText(m_cells[row].source);
    m_output->setPlainText(m_cells[row].lastOutput);
    m_suppressSourceSignal = false;
}

void NotebookPanel::onSourceChanged()
{
    if (m_suppressSourceSignal) return;
    persistCurrentSource();
    if (auto* it = itemForRow(currentRow()); it)
        it->setText(cellHeadline(m_cells[currentRow()]));
}

void NotebookPanel::onAddCode()
{
    const int row = qMax(currentRow(), 0);
    m_cells.insert(row + 1, { QStringLiteral("code"), QString(), QString() });
    rebuildList();
    m_list->setCurrentRow(row + 1);
}
void NotebookPanel::onAddMarkdown()
{
    const int row = qMax(currentRow(), 0);
    m_cells.insert(row + 1, { QStringLiteral("markdown"), QString(), QString() });
    rebuildList();
    m_list->setCurrentRow(row + 1);
}
void NotebookPanel::onDeleteCell()
{
    const int row = currentRow();
    if (row < 0 || row >= m_cells.size()) return;
    m_cells.removeAt(row);
    rebuildList();
}
void NotebookPanel::onMoveUp()
{
    const int row = currentRow();
    if (row <= 0) return;
    m_cells.swapItemsAt(row, row - 1);
    rebuildList();
    m_list->setCurrentRow(row - 1);
}
void NotebookPanel::onMoveDown()
{
    const int row = currentRow();
    if (row < 0 || row + 1 >= m_cells.size()) return;
    m_cells.swapItemsAt(row, row + 1);
    rebuildList();
    m_list->setCurrentRow(row + 1);
}

void NotebookPanel::onRunCell()
{
    const int row = currentRow();
    if (row < 0 || row >= m_cells.size()) return;
    persistCurrentSource();
    Cell& c = m_cells[row];
    if (c.type != QStringLiteral("code")) {
        m_output->setPlainText(tr("(célula markdown — nada para executar)"));
        return;
    }
    m_status->setText(tr("Executando…"));

    QProcess p;
    p.setProcessChannelMode(QProcess::MergedChannels);
    p.start(QStringLiteral("python3"), { QStringLiteral("-") });
    if (!p.waitForStarted(2000)) {
        m_output->setPlainText(tr("Erro: python3 não encontrado."));
        m_status->clear();
        return;
    }
    p.write(c.source.toUtf8());
    p.closeWriteChannel();
    if (!p.waitForFinished(30000)) { p.kill(); p.waitForFinished(500); }
    const QByteArray out = p.readAllStandardOutput();
    c.lastOutput = QString::fromUtf8(out);
    m_output->setPlainText(c.lastOutput);
    m_status->setText(tr("Terminou (exit=%1).").arg(p.exitCode()));
}

bool NotebookPanel::openFile(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Notebook"), tr("Não consegui abrir %1").arg(path));
        return false;
    }
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    f.close();
    if (err.error != QJsonParseError::NoError) {
        QMessageBox::warning(this, tr("Notebook"), tr("JSON inválido: %1").arg(err.errorString()));
        return false;
    }
    m_cells.clear();
    for (const QJsonValue& v : doc.object().value(QStringLiteral("cells")).toArray()) {
        const QJsonObject co = v.toObject();
        Cell c;
        c.type = co.value(QStringLiteral("cell_type")).toString(QStringLiteral("code"));
        // `source` may be a string OR an array of lines (the IPython convention).
        const QJsonValue src = co.value(QStringLiteral("source"));
        if (src.isArray()) {
            QStringList parts;
            for (const QJsonValue& s : src.toArray()) parts << s.toString();
            c.source = parts.join(QString());
        } else {
            c.source = src.toString();
        }
        m_cells.append(c);
    }
    if (m_cells.isEmpty()) m_cells.append({ QStringLiteral("code"), QString(), QString() });
    m_filePath = path;
    rebuildList();
    m_list->setCurrentRow(0);
    setWindowTitle(tr("Notebook — %1").arg(QFileInfo(path).fileName()));
    return true;
}

bool NotebookPanel::save()
{
    if (m_filePath.isEmpty()) return saveAs();
    persistCurrentSource();
    QJsonArray cells;
    for (const Cell& c : m_cells) {
        QJsonObject o;
        o.insert(QStringLiteral("cell_type"), c.type);
        o.insert(QStringLiteral("source"), c.source);
        o.insert(QStringLiteral("metadata"), QJsonObject());
        if (c.type == QStringLiteral("code")) {
            o.insert(QStringLiteral("execution_count"), QJsonValue());
            o.insert(QStringLiteral("outputs"),         QJsonArray());
        }
        cells.append(o);
    }
    QJsonObject root;
    root.insert(QStringLiteral("cells"), cells);
    root.insert(QStringLiteral("nbformat"), 4);
    root.insert(QStringLiteral("nbformat_minor"), 5);
    QJsonObject meta;
    QJsonObject ki; ki.insert(QStringLiteral("name"), QStringLiteral("python3"));
    meta.insert(QStringLiteral("kernelspec"), ki);
    root.insert(QStringLiteral("metadata"), meta);

    QFile f(m_filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    f.close();
    m_status->setText(tr("Salvo em %1").arg(m_filePath));
    return true;
}

bool NotebookPanel::saveAs()
{
    const QString p = QFileDialog::getSaveFileName(this, tr("Salvar notebook"),
        m_filePath.isEmpty() ? QStringLiteral("notebook.ipynb") : m_filePath,
        tr("Jupyter notebooks (*.ipynb)"));
    if (p.isEmpty()) return false;
    m_filePath = p;
    return save();
}

void NotebookPanel::onOpen()
{
    const QString p = QFileDialog::getOpenFileName(this, tr("Abrir notebook"),
        QString(), tr("Jupyter notebooks (*.ipynb)"));
    if (!p.isEmpty()) openFile(p);
}
void NotebookPanel::onSave() { save(); }
