#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QPointer>

class QMainWindow;

// LayoutPresets (M26) — salva/restaura QMainWindow::saveState / saveGeometry
// como named presets em QSettings/Layouts/<name>.
class LayoutPresets : public QObject {
    Q_OBJECT
public:
    explicit LayoutPresets(QMainWindow* host, QObject* parent = nullptr);

    QStringList list() const;
    bool save(const QString& name);
    bool apply(const QString& name);
    void remove(const QString& name);

private:
    QPointer<QMainWindow> m_host;
};
