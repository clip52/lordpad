#pragma once

#include <QDockWidget>

class QTreeWidget;
class QPushButton;
class QLabel;
class ScintillaEdit;

// SecretScannerPanel (M43) — varre buffer ativo procurando padrões comuns
// de secrets (AWS, GitHub PATs, generic API keys, JWTs, RSA priv).
class SecretScannerPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit SecretScannerPanel(QWidget* parent = nullptr);
    void setActiveEditor(ScintillaEdit* ed) { m_editor = ed; }
signals:
    void gotoLineRequested(int line);
private slots:
    void onScan();
    void onItemActivated(class QTreeWidgetItem* it, int col);
private:
    ScintillaEdit* m_editor = nullptr;
    QPushButton*   m_scanBtn = nullptr;
    QTreeWidget*   m_findings = nullptr;
    QLabel*        m_status = nullptr;
};
