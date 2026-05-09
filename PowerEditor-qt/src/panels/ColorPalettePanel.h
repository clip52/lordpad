#pragma once

#include <QDockWidget>
#include <QColor>

class QLabel;
class QLineEdit;
class QPushButton;
class QListWidget;
class QListWidgetItem;

// ColorPalettePanel (M18) — picker visual + paleta persistente + WCAG ratio.
// Botão "Inserir hex" pasta o #RRGGBB no editor ativo.
class ColorPalettePanel : public QDockWidget {
    Q_OBJECT
public:
    explicit ColorPalettePanel(QWidget* parent = nullptr);

signals:
    // MainWindow conecta isso pra inserir o hex no caret do editor ativo.
    void insertHexRequested(const QString& hex);

private slots:
    void onPickColor();
    void onAddToPalette();
    void onRemoveFromPalette();
    void onPaletteItemActivated(QListWidgetItem* it);
    void onUpdateRatio();
    void onCopyHex();
    void onInsertHex();

private:
    void persistPalette() const;
    void loadPalette();
    void rerender();
    static double luminance(const QColor& c);
    static double contrast(const QColor& a, const QColor& b);

    QColor       m_color   = QColor("#3b82f6");
    QColor       m_against = QColor("#ffffff");

    QLabel*      m_swatch = nullptr;
    QLineEdit*   m_hexEdit = nullptr;
    QPushButton* m_pickBtn = nullptr;
    QPushButton* m_copyBtn = nullptr;
    QPushButton* m_insertBtn = nullptr;
    QPushButton* m_addBtn = nullptr;
    QPushButton* m_delBtn = nullptr;
    QPushButton* m_againstBtn = nullptr;
    QListWidget* m_palette = nullptr;
    QLabel*      m_ratio = nullptr;
};
