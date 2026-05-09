#pragma once

#include <QDockWidget>

class QTabWidget;
class QPlainTextEdit;
class QLineEdit;
class QPushButton;
class QSpinBox;
class QComboBox;
class QLabel;
class QCheckBox;

// DevToolsPanel (M22) — utilitário swiss-army com tabs:
//   JWT decoder | Base64 | URL encode/decode | UUID v4 | Hash (md5/sha-1/256/512)
//   | Lorem ipsum | Password generator
class DevToolsPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit DevToolsPanel(QWidget* parent = nullptr);

private slots:
    void onJwtDecode();
    void onB64Encode();
    void onB64Decode();
    void onUrlEncode();
    void onUrlDecode();
    void onUuidGen();
    void onHashCompute();
    void onLoremGen();
    void onPasswordGen();

private:
    QTabWidget* m_tabs = nullptr;

    QPlainTextEdit* m_jwtIn = nullptr;
    QPlainTextEdit* m_jwtOut = nullptr;

    QPlainTextEdit* m_b64In  = nullptr;
    QPlainTextEdit* m_b64Out = nullptr;

    QPlainTextEdit* m_urlIn  = nullptr;
    QPlainTextEdit* m_urlOut = nullptr;

    QPlainTextEdit* m_uuidOut = nullptr;
    QSpinBox*       m_uuidCount = nullptr;

    QComboBox*      m_hashAlg = nullptr;
    QPlainTextEdit* m_hashIn = nullptr;
    QLineEdit*      m_hashOut = nullptr;

    QSpinBox*       m_loremPara = nullptr;
    QPlainTextEdit* m_loremOut = nullptr;

    QSpinBox*       m_pwLen = nullptr;
    QCheckBox*      m_pwUpper = nullptr;
    QCheckBox*      m_pwDigits = nullptr;
    QCheckBox*      m_pwSymbols = nullptr;
    QLineEdit*      m_pwOut = nullptr;
};
