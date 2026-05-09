#pragma once

#include <QDockWidget>
#include <QHash>
#include <QPointer>

class QLineEdit;
class QPushButton;
class QTreeWidget;
class QLabel;
class QProcess;

// TunnelManagerPanel (M68) — gerencia túneis SSH -L em background.
class TunnelManagerPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit TunnelManagerPanel(QWidget* parent = nullptr);
    ~TunnelManagerPanel() override;
private slots:
    void onAdd();
    void onRemove();
    void onStart();
    void onStop();
private:
    void persist() const;
    void load();
    QString rowKey(int row) const;

    QLineEdit*   m_name = nullptr;
    QLineEdit*   m_host = nullptr;       // ex user@host
    QLineEdit*   m_local = nullptr;      // ex 5432
    QLineEdit*   m_remote = nullptr;     // ex db.intra:5432
    QPushButton* m_addBtn = nullptr;
    QPushButton* m_delBtn = nullptr;
    QPushButton* m_startBtn = nullptr;
    QPushButton* m_stopBtn  = nullptr;
    QTreeWidget* m_tree = nullptr;
    QLabel*      m_status = nullptr;
    QHash<QString, QProcess*> m_procs;
};
