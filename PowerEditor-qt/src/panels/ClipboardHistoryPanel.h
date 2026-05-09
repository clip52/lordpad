#pragma once

#include <QDockWidget>

class QListWidget;
class QListWidgetItem;
class QClipboard;
class QPushButton;

// ClipboardHistoryPanel (M25) — escuta mudanças no QClipboard, mantém um
// histórico circular de até 50 entradas. Click na entrada copia de volta
// pro clipboard; double-click insere no editor ativo.
class ClipboardHistoryPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit ClipboardHistoryPanel(QWidget* parent = nullptr);

signals:
    void insertTextRequested(const QString& text);

private slots:
    void onClipboardChanged();
    void onItemActivated(QListWidgetItem* it);
    void onClear();

private:
    QListWidget* m_list = nullptr;
    QPushButton* m_clearBtn = nullptr;
};
