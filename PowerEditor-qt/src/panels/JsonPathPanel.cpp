#include "JsonPathPanel.h"

#include <QFontDatabase>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

#include <ScintillaEdit.h>

namespace {

// Tokenize ".a.b[2].c[*]" into ["a", "b", "[2]", "c", "[*]"]. Empty input
// returns "$" (the whole document).
QStringList tokenizePath(const QString& path)
{
    QStringList out;
    QString s = path.trimmed();
    if (s.startsWith('$')) s = s.mid(1);
    int i = 0;
    while (i < s.size()) {
        const QChar c = s[i];
        if (c == '.') { ++i; continue; }
        if (c == '[') {
            const int end = s.indexOf(']', i + 1);
            if (end < 0) break;
            out << s.mid(i, end - i + 1);
            i = end + 1;
            continue;
        }
        // Plain identifier.
        int j = i;
        while (j < s.size() && s[j] != '.' && s[j] != '[') ++j;
        out << s.mid(i, j - i);
        i = j;
    }
    return out;
}

QJsonValue applyToken(const QJsonValue& v, const QString& tok, bool& ok)
{
    ok = true;
    if (tok.isEmpty()) return v;
    if (tok.startsWith('[') && tok.endsWith(']')) {
        const QString inner = tok.mid(1, tok.size() - 2);
        if (inner == QStringLiteral("*")) {
            return v;   // wildcards handled by caller
        }
        bool isInt = false;
        const int idx = inner.toInt(&isInt);
        if (isInt && v.isArray()) {
            const QJsonArray arr = v.toArray();
            if (idx < 0 || idx >= arr.size()) { ok = false; return {}; }
            return arr.at(idx);
        }
        // Quoted key, e.g. ["foo bar"]
        if (inner.size() >= 2 && inner.startsWith('"') && inner.endsWith('"')) {
            const QString key = inner.mid(1, inner.size() - 2);
            if (v.isObject()) return v.toObject().value(key);
        }
        ok = false; return {};
    }
    if (v.isObject()) return v.toObject().value(tok);
    ok = false; return {};
}

QJsonArray applyPath(const QJsonValue& root, const QStringList& tokens, bool& ok)
{
    QJsonArray cur; cur.append(root);
    for (const QString& tok : tokens) {
        QJsonArray next;
        if (tok == QStringLiteral("[*]")) {
            for (const QJsonValue& v : cur) {
                if (v.isArray()) for (const QJsonValue& e : v.toArray()) next.append(e);
                else if (v.isObject()) for (const QJsonValue& e : v.toObject()) next.append(e);
            }
        } else {
            for (const QJsonValue& v : cur) {
                bool localOk = true;
                const QJsonValue n = applyToken(v, tok, localOk);
                if (localOk && !n.isUndefined()) next.append(n);
            }
        }
        cur = next;
        if (cur.isEmpty()) { ok = (tokens.size() == 0); return cur; }
    }
    ok = true;
    return cur;
}

}

JsonPathPanel::JsonPathPanel(QWidget* parent) : QDockWidget(tr("JSON path"), parent)
{
    setObjectName(QStringLiteral("JsonPathPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);

    m_path   = new QLineEdit(root);
    m_path->setPlaceholderText(QStringLiteral(".foo.bar[0].baz   ou   $.items[*].name"));
    m_path->setFont(mono);
    auto* applyBtn = new QPushButton(tr("Aplicar"), root);
    auto* row = new QHBoxLayout();
    row->addWidget(new QLabel(tr("Path:"), root));
    row->addWidget(m_path, 1);
    row->addWidget(applyBtn);

    m_output = new QPlainTextEdit(root);
    m_output->setReadOnly(true);
    m_output->setFont(mono);

    m_status = new QLabel(root);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addLayout(row);
    lay->addWidget(m_output, 1);
    lay->addWidget(m_status);
    setWidget(root);

    connect(applyBtn, &QPushButton::clicked, this, &JsonPathPanel::onApply);
    connect(m_path,   &QLineEdit::returnPressed, this, &JsonPathPanel::onApply);
}

void JsonPathPanel::setActiveEditor(ScintillaEdit* editor) { m_editor = editor; }

void JsonPathPanel::onApply()
{
    if (!m_editor) { m_status->setText(tr("Sem editor.")); return; }
    QByteArray bytes = m_editor->getText(m_editor->textLength() + 1);
    QJsonParseError pe{};
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &pe);
    if (pe.error != QJsonParseError::NoError) {
        m_status->setText(tr("JSON inválido: %1").arg(pe.errorString()));
        m_output->clear();
        return;
    }
    QJsonValue rootV = doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object());
    const QStringList tokens = tokenizePath(m_path->text());
    bool ok = true;
    const QJsonArray result = applyPath(rootV, tokens, ok);

    QJsonDocument outDoc;
    if (result.size() == 1) {
        const QJsonValue v = result.at(0);
        if (v.isArray())       outDoc = QJsonDocument(v.toArray());
        else if (v.isObject()) outDoc = QJsonDocument(v.toObject());
        else {
            m_output->setPlainText(QString::fromUtf8(QJsonDocument::fromVariant(v.toVariant()).toJson()));
            m_status->setText(tr("1 resultado escalar"));
            return;
        }
    } else {
        outDoc = QJsonDocument(result);
    }
    m_output->setPlainText(QString::fromUtf8(outDoc.toJson(QJsonDocument::Indented)));
    m_status->setText(tr("%1 resultado(s)").arg(result.size()));
}
