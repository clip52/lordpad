#pragma once

#include <QDockWidget>

class QLineEdit;
class QPlainTextEdit;
class QTableWidget;
class QPushButton;
class QLabel;

// JsonLinesPanel (M23) — viewer pra arquivos NDJSON / JSONL.
// Carrega arquivo, parse linha a linha, mostra cada record como row na tabela
// (1ª passada coleta superset de chaves; cada record exibe valor por chave).
class JsonLinesPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit JsonLinesPanel(QWidget* parent = nullptr);

private slots:
    void onOpen();
    void onFilter(const QString& text);
    void onRowActivated(int row, int col);

private:
    QPushButton*  m_openBtn = nullptr;
    QLineEdit*    m_filter = nullptr;
    QTableWidget* m_table = nullptr;
    QPlainTextEdit* m_detail = nullptr;
    QLabel*       m_status = nullptr;
    QStringList   m_rawLines;
};
