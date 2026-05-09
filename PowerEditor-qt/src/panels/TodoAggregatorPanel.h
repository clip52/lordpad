#pragma once

#include <QDockWidget>

class QLineEdit;
class QPushButton;
class QTreeWidget;
class QLabel;

// TodoAggregatorPanel (M51) — varre dir por TODO/FIXME/XXX/HACK markers.
class TodoAggregatorPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit TodoAggregatorPanel(QWidget* parent = nullptr);
signals:
    void openFileAtLine(const QString& path, int line);
public slots:
    void scan(const QString& dir);
private slots:
    void onPickDir();
    void onScan();
    void onItemActivated();
private:
    QLineEdit*   m_dirEdit = nullptr;
    QPushButton* m_pickBtn = nullptr;
    QPushButton* m_scanBtn = nullptr;
    QTreeWidget* m_tree    = nullptr;
    QLabel*      m_status  = nullptr;
};
