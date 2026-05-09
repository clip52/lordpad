#pragma once

#include <QDockWidget>
#include <QPointer>

class QLineEdit;
class QPushButton;
class QTreeWidget;
class QLabel;
class QTcpSocket;

// PortScanPanel (M66) — listeners locais (ss) + scan TCP de host:portas.
class PortScanPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit PortScanPanel(QWidget* parent = nullptr);
private slots:
    void onLocalListeners();
    void onScanRange();
private:
    QLineEdit*   m_host = nullptr;
    QLineEdit*   m_ports = nullptr;
    QPushButton* m_localBtn = nullptr;
    QPushButton* m_scanBtn = nullptr;
    QTreeWidget* m_tree = nullptr;
    QLabel*      m_status = nullptr;
};
