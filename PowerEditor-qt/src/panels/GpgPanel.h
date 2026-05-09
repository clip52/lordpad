#pragma once

#include <QDockWidget>

class QPushButton;
class QPlainTextEdit;
class QLineEdit;
class QLabel;
class QTreeWidget;
class ScintillaEdit;

// GpgPanel (M63) — encrypt/decrypt/sign/verify + lista chaves via gpg CLI.
class GpgPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit GpgPanel(QWidget* parent = nullptr);
    void setActiveEditor(ScintillaEdit* ed) { m_editor = ed; }
private slots:
    void onListKeys();
    void onEncrypt();
    void onDecrypt();
    void onSign();
    void onVerify();
private:
    ScintillaEdit*  m_editor = nullptr;
    QLineEdit*      m_recipient = nullptr;
    QPushButton*    m_listBtn = nullptr;
    QPushButton*    m_encBtn = nullptr;
    QPushButton*    m_decBtn = nullptr;
    QPushButton*    m_signBtn = nullptr;
    QPushButton*    m_verifyBtn = nullptr;
    QTreeWidget*    m_keys = nullptr;
    QPlainTextEdit* m_out = nullptr;
    QLabel*         m_status = nullptr;
};
