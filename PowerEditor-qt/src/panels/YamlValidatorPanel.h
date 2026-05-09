#pragma once

#include <QDockWidget>
#include <QPointer>

class QPushButton;
class QPlainTextEdit;
class QLabel;
class QProcess;
class ScintillaEdit;

// YamlValidatorPanel (M53) — valida YAML do buffer com python3 -c yaml.safe_load.
class YamlValidatorPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit YamlValidatorPanel(QWidget* parent = nullptr);
    void setActiveEditor(ScintillaEdit* ed) { m_editor = ed; }
public slots:
    void onValidate();
private:
    ScintillaEdit*  m_editor = nullptr;
    QPushButton*    m_btn = nullptr;
    QPlainTextEdit* m_out = nullptr;
    QLabel*         m_status = nullptr;
};
