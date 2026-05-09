#pragma once

#include <QDockWidget>

class QLineEdit;
class QPlainTextEdit;
class QCheckBox;
class QTableWidget;
class QLabel;

// RegexTesterPanel (M18) — tester ao vivo: pattern + texto → tabela de matches
// com índices, captura groups e replace preview.
class RegexTesterPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit RegexTesterPanel(QWidget* parent = nullptr);

private slots:
    void onRecheck();

private:
    QLineEdit*      m_pattern = nullptr;
    QLineEdit*      m_replace = nullptr;
    QCheckBox*      m_caseInsensitive = nullptr;
    QCheckBox*      m_multiline = nullptr;
    QCheckBox*      m_dotAll = nullptr;
    QPlainTextEdit* m_subject = nullptr;
    QPlainTextEdit* m_replacePreview = nullptr;
    QTableWidget*   m_matches = nullptr;
    QLabel*         m_status = nullptr;
};
