#pragma once

#include <QDockWidget>

class QPlainTextEdit;
class QPushButton;
class QLabel;

// CronEditorPanel (M35) — `crontab -l` lê → editor → `crontab -` salva.
class CronEditorPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit CronEditorPanel(QWidget* parent = nullptr);

private slots:
    void onLoad();
    void onSave();

private:
    QPlainTextEdit* m_text = nullptr;
    QPushButton*    m_loadBtn = nullptr;
    QPushButton*    m_saveBtn = nullptr;
    QLabel*         m_status = nullptr;
};
