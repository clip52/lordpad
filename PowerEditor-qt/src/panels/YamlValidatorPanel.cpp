#include "YamlValidatorPanel.h"

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWidget>

#include <ScintillaEdit.h>

YamlValidatorPanel::YamlValidatorPanel(QWidget* parent) : QDockWidget(tr("YAML"), parent)
{
    setObjectName(QStringLiteral("YamlValidatorPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_btn = new QPushButton(tr("Validar buffer"), root);
    m_out = new QPlainTextEdit(root); m_out->setReadOnly(true); m_out->setFont(mono);
    m_status = new QLabel(root);

    auto* row = new QHBoxLayout();
    row->addWidget(m_btn);
    row->addStretch(1);
    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4,4,4,4);
    lay->addLayout(row);
    lay->addWidget(m_out, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(m_btn, &QPushButton::clicked, this, &YamlValidatorPanel::onValidate);
}

void YamlValidatorPanel::onValidate()
{
    if (!m_editor) { m_status->setText(tr("Sem editor.")); return; }
    QByteArray bytes = m_editor->getText(m_editor->textLength() + 1);
    if (!bytes.isEmpty() && bytes.endsWith('\0')) bytes.chop(1);

    const QString py = QStandardPaths::findExecutable(QStringLiteral("python3"));
    if (py.isEmpty()) { m_status->setText(tr("python3 não encontrado.")); return; }

    QProcess p;
    p.start(py, { QStringLiteral("-c"),
        QStringLiteral("import sys, yaml\n"
                       "try:\n"
                       "    docs = list(yaml.safe_load_all(sys.stdin))\n"
                       "    print(f'OK — {len(docs)} documento(s)')\n"
                       "except yaml.YAMLError as e:\n"
                       "    print(f'YAML error: {e}', file=sys.stderr)\n"
                       "    sys.exit(1)\n") });
    p.write(bytes);
    p.closeWriteChannel();
    if (!p.waitForFinished(15000)) { p.kill(); m_status->setText(tr("Timeout.")); return; }
    const QByteArray out = p.readAllStandardOutput();
    const QByteArray err = p.readAllStandardError();
    if (p.exitCode() == 0) {
        m_out->setPlainText(QString::fromUtf8(out));
        m_status->setText(tr("Válido."));
    } else {
        m_out->setPlainText(QString::fromUtf8(err));
        m_status->setText(tr("Erro de validação."));
    }
}
