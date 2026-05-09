#pragma once

#include <QDockWidget>
#include <QString>

class QPlainTextEdit;
class QPushButton;
class QLabel;
class QComboBox;

// QrPanel — utilitário M12. Encoda texto pra QR code via subprocess
// `qrencode` (do pacote homônimo). Sem `qrencode` no PATH, mostra a sequência
// de instalação típica de cada distro.
class QrPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit QrPanel(QWidget* parent = nullptr);

private slots:
    void onGenerate();
    void onSaveAs();
    void onCopyImage();

private:
    bool runQrencode(const QString& text, const QString& outPath, QString* outErr) const;

    QPlainTextEdit* m_input  = nullptr;
    QComboBox*      m_eccLevel = nullptr;
    QPushButton*    m_genBtn  = nullptr;
    QPushButton*    m_saveBtn = nullptr;
    QPushButton*    m_copyBtn = nullptr;
    QLabel*         m_image   = nullptr;
    QLabel*         m_status  = nullptr;
    QString         m_lastPng;        // tmp file path of the most recent QR
};
