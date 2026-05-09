#pragma once

#include <QDockWidget>
#include <QDate>
#include <QHash>
#include <QString>

class QCalendarWidget;
class QPlainTextEdit;
class QLabel;

// CalendarPanel — utilitário M12. QCalendarWidget com locale pt-BR, feriados
// nacionais brasileiros pintados em vermelho na cor de fim-de-semana, e um
// editor de notas por data. As notas vivem em QSettings sob "Calendar/notes".
class CalendarPanel : public QDockWidget {
    Q_OBJECT
public:
    explicit CalendarPanel(QWidget* parent = nullptr);

private slots:
    void onDateClicked(const QDate& d);
    void onNotesChanged();

private:
    void paintBrazilianHolidays();
    void loadNotesForDate(const QDate& d);
    QString notesKey(const QDate& d) const;

    QCalendarWidget* m_cal     = nullptr;
    QPlainTextEdit*  m_notes   = nullptr;
    QLabel*          m_holiday = nullptr;
    QDate            m_currentDate;
};
