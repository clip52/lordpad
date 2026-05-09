#pragma once

#include <QDialog>
#include <QPointer>

class KeybindingsManager;
class QLineEdit;
class QTableWidget;
class QTableWidgetItem;
class QPushButton;

// Dialog for browsing + editing keybindings. Shows a table of (Action,
// Default, Custom) and lets the user record a new shortcut directly on the
// row by clicking the Custom column and typing the desired key combo.
class KeybindingsDialog : public QDialog {
    Q_OBJECT
public:
    KeybindingsDialog(KeybindingsManager* manager, QWidget* parent = nullptr);

private slots:
    void onFilterChanged(const QString& text);
    void onCellChanged(int row, int column);
    void onResetAll();
    void onResetSelected();

private:
    void rebuild();
    bool isShortcutItem(int column) const { return column == 2; }

    QPointer<KeybindingsManager> m_manager;
    QLineEdit*    m_filter = nullptr;
    QTableWidget* m_table  = nullptr;
    QPushButton*  m_resetAll  = nullptr;
    QPushButton*  m_resetThis = nullptr;
    bool          m_suppressEdits = false;
};
