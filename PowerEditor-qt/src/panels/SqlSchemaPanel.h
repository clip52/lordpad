#pragma once

#include <QDockWidget>
#include <QPointer>
#include <QStringList>

class QGraphicsScene;
class QGraphicsView;
class QPushButton;
class QLabel;
class ScintillaEdit;

// SqlSchemaPanel (M49) — parseia CREATE TABLE no buffer e desenha ER simplificado.
class SqlSchemaPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit SqlSchemaPanel(QWidget* parent = nullptr);
    void setActiveEditor(ScintillaEdit* editor);
public slots:
    void onParseAndDraw();
    void onSavePng();
private:
    struct Table {
        QString name;
        QStringList columns;     // "name TYPE" string p/ render
        QStringList foreignKeys; // "col -> tableRef.colRef"
    };
    QList<Table> parse(const QString& sql) const;
    void draw(const QList<Table>& tables);

    QPointer<ScintillaEdit> m_editor;
    QGraphicsScene* m_scene = nullptr;
    QGraphicsView*  m_view  = nullptr;
    QPushButton*    m_parseBtn = nullptr;
    QPushButton*    m_savePngBtn = nullptr;
    QLabel*         m_status = nullptr;
};
