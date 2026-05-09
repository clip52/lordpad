#include "CalendarPanel.h"

#include <QCalendarWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPlainTextEdit>
#include <QSettings>
#include <QSplitter>
#include <QTextCharFormat>
#include <QVBoxLayout>
#include <QWidget>

namespace {

// Brazilian fixed-date national holidays. Easter-derived ones (Carnaval,
// Sexta-Feira Santa, Corpus Christi) require date arithmetic — added below
// in computeMovableHolidays. The names are pt-BR per local convention.
struct FixedHoliday { int month; int day; const char* name; };
constexpr FixedHoliday kFixed[] = {
    { 1,  1,  "Confraternização Universal" },
    { 4, 21,  "Tiradentes" },
    { 5,  1,  "Dia do Trabalhador" },
    { 9,  7,  "Independência do Brasil" },
    {10, 12,  "Nossa Senhora Aparecida" },
    {11,  2,  "Finados" },
    {11, 15,  "Proclamação da República" },
    {11, 20,  "Dia da Consciência Negra" },
    {12, 25,  "Natal" },
};

// Easter Sunday for Gregorian year — Anonymous Gregorian algorithm.
QDate easterSunday(int year)
{
    const int a = year % 19;
    const int b = year / 100;
    const int c = year % 100;
    const int d = b / 4;
    const int e = b % 4;
    const int f = (b + 8) / 25;
    const int g = (b - f + 1) / 3;
    const int h = (19 * a + b - d - g + 15) % 30;
    const int i = c / 4;
    const int k = c % 4;
    const int l = (32 + 2 * e + 2 * i - h - k) % 7;
    const int m = (a + 11 * h + 22 * l) / 451;
    const int month = (h + l - 7 * m + 114) / 31;
    const int day   = ((h + l - 7 * m + 114) % 31) + 1;
    return QDate(year, month, day);
}

QHash<QDate, QString> brazilianHolidaysFor(int year)
{
    QHash<QDate, QString> out;
    for (const FixedHoliday& h : kFixed) {
        out.insert(QDate(year, h.month, h.day), QString::fromUtf8(h.name));
    }
    const QDate easter = easterSunday(year);
    out.insert(easter.addDays(-48), QStringLiteral("Carnaval (segunda)"));
    out.insert(easter.addDays(-47), QStringLiteral("Carnaval (terça)"));
    out.insert(easter.addDays(-2),  QStringLiteral("Sexta-Feira Santa"));
    out.insert(easter,              QStringLiteral("Páscoa"));
    out.insert(easter.addDays(60),  QStringLiteral("Corpus Christi"));
    return out;
}

} // namespace

CalendarPanel::CalendarPanel(QWidget* parent)
    : QDockWidget(tr("Calendário"), parent)
{
    setObjectName(QStringLiteral("CalendarPanel"));
    setAllowedAreas(Qt::AllDockWidgetAreas);

    auto* root = new QWidget(this);
    m_cal = new QCalendarWidget(root);
    m_cal->setLocale(QLocale(QLocale::Portuguese, QLocale::Brazil));
    m_cal->setFirstDayOfWeek(Qt::Sunday);
    m_cal->setGridVisible(true);
    m_cal->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

    m_holiday = new QLabel(root);
    m_holiday->setStyleSheet(QStringLiteral("QLabel { color: #d04040; font-weight: bold; }"));
    m_holiday->setWordWrap(true);

    m_notes = new QPlainTextEdit(root);
    m_notes->setPlaceholderText(tr("Notas para a data selecionada…"));
    m_notes->setMaximumBlockCount(2000);

    auto* lay = new QVBoxLayout(root);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->addWidget(m_cal);
    lay->addWidget(m_holiday);
    lay->addWidget(new QLabel(tr("Notas:"), root));
    lay->addWidget(m_notes, 1);
    setWidget(root);

    paintBrazilianHolidays();

    connect(m_cal, &QCalendarWidget::clicked, this, &CalendarPanel::onDateClicked);
    connect(m_cal, &QCalendarWidget::currentPageChanged, this,
            [this](int /*y*/, int /*m*/) { paintBrazilianHolidays(); });
    connect(m_notes, &QPlainTextEdit::textChanged, this, &CalendarPanel::onNotesChanged);

    onDateClicked(m_cal->selectedDate());
}

void CalendarPanel::paintBrazilianHolidays()
{
    if (!m_cal) return;

    QTextCharFormat plain;
    QTextCharFormat holiday;
    holiday.setForeground(QColor("#d04040"));
    holiday.setFontWeight(QFont::Bold);
    holiday.setToolTip(QString());

    // Repaint the visible year(s) — the user may have flipped to a year we
    // haven't decorated yet. Cheap enough to redo every page change.
    const int currentYear = m_cal->yearShown();
    for (int y : { currentYear - 1, currentYear, currentYear + 1 }) {
        const auto holidays = brazilianHolidaysFor(y);
        for (auto it = holidays.constBegin(); it != holidays.constEnd(); ++it) {
            QTextCharFormat fmt = holiday;
            fmt.setToolTip(it.value());
            m_cal->setDateTextFormat(it.key(), fmt);
        }
    }
}

void CalendarPanel::onDateClicked(const QDate& d)
{
    m_currentDate = d;

    // Holiday banner.
    const auto holidays = brazilianHolidaysFor(d.year());
    auto it = holidays.constFind(d);
    m_holiday->setText(it == holidays.constEnd() ? QString() : it.value());

    loadNotesForDate(d);
}

QString CalendarPanel::notesKey(const QDate& d) const
{
    return QStringLiteral("Calendar/notes/%1").arg(d.toString(Qt::ISODate));
}

void CalendarPanel::loadNotesForDate(const QDate& d)
{
    QSignalBlocker block(m_notes);
    QSettings s;
    m_notes->setPlainText(s.value(notesKey(d)).toString());
}

void CalendarPanel::onNotesChanged()
{
    if (!m_currentDate.isValid()) return;
    QSettings s;
    const QString text = m_notes->toPlainText();
    if (text.isEmpty()) s.remove(notesKey(m_currentDate));
    else                s.setValue(notesKey(m_currentDate), text);
}
