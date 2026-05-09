#pragma once

#include <QDockWidget>

class QTreeWidget;
class QPushButton;
class QSpinBox;
class QLabel;
class ScintillaEdit;

// CodeClonesPanel (M40) — encontra blocos repetidos de N linhas no buffer.
// Algoritmo: hash de janela de N linhas (trimmed), agrupa hashes com
// mais de 1 ocorrência. Útil pra spotting de DRY violations triviais.
class CodeClonesPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit CodeClonesPanel(QWidget* parent = nullptr);
    void setActiveEditor(ScintillaEdit* ed) { m_editor = ed; }
signals:
    void gotoLineRequested(int line);
private slots:
    void onAnalyze();
    void onItemActivated(class QTreeWidgetItem* it, int);
private:
    ScintillaEdit* m_editor = nullptr;
    QSpinBox*    m_window = nullptr;
    QPushButton* m_btn = nullptr;
    QTreeWidget* m_tree = nullptr;
    QLabel*      m_status = nullptr;
};
