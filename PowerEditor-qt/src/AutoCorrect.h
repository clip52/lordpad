#pragma once

#include <QObject>
#include <QHash>
#include <QSet>
#include <QString>

class ScintillaEdit;

// AutoCorrect (M27) — substitui erros comuns ao digitar espaço/Enter.
// Dicionário em QSettings/AutoCorrect/dict (par "errado":"certo").
class AutoCorrect : public QObject {
    Q_OBJECT
public:
    explicit AutoCorrect(QObject* parent = nullptr);

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool on);

    void attach(ScintillaEdit* editor);
    void detach(ScintillaEdit* editor);

    QHash<QString, QString> dict() const { return m_dict; }
    void setEntry(const QString& wrong, const QString& right);
    void removeEntry(const QString& wrong);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void load();
    void persist() const;
    void seedDefaults();
    void tryCorrectAtCaret(ScintillaEdit* sci);

    bool m_enabled = true;
    QHash<QString, QString> m_dict;
    QSet<ScintillaEdit*> m_editors;
};
