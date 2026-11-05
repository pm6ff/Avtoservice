#include "Common.h"
#include <QDate>
#include <QtGlobal>

QStringList Role::all() { return {Admin, Reception, Master}; }
QStringList Status::all() { return {Accepted, InWork, Ready, Issued}; }
QStringList Staff::all() { return {Working, DayOff}; }
QStringList Kind::all() { return {Service, Part}; }

Session& Session::instance() {
    static Session s;
    return s;
}

QString Fmt::money(double v) {
    const bool whole = qAbs(v - qRound64(v)) < 0.005;
    QString s = QString::number(qAbs(v), 'f', whole ? 0 : 2);
    const int dot = s.indexOf(QLatin1Char('.'));
    const int intLen = dot < 0 ? s.size() : dot;
    for (int i = intLen - 3; i > 0; i -= 3)
        s.insert(i, QChar(0x00A0));
    s.replace(QLatin1Char('.'), QLatin1Char(','));
    return (v < 0 ? QStringLiteral("-") : QString()) + s + QStringLiteral(" ₽");
}

QString Fmt::shortName(const QString& fullName) {
    const QString s = fullName.simplified();
    if (s.contains(QLatin1Char('.')))
        return s;
    const QStringList p = s.split(QLatin1Char(' '));
    if (p.size() < 2)
        return s;
    QString r = p[0] + QLatin1Char(' ') + p[1].left(1) + QLatin1Char('.');
    if (p.size() > 2)
        r += p[2].left(1) + QLatin1Char('.');
    return r;
}

QString Fmt::dateUi(const QString& iso) {
    return QDate::fromString(iso, Qt::ISODate).toString(QStringLiteral("dd.MM.yyyy"));
}

// Приводит символ к «поисковому» виду: ё -> е, а русские буквы, похожие на латинские, -> латиница.
// Это нужно, чтобы госномер «А123ВС» находился и по русским, и по латинским буквам.
static QChar foldChar(QChar ch) {
    switch (ch.unicode()) {
    case 0x0451: return QLatin1Char('e');   // ё
    case 0x0430: return QLatin1Char('a');   // а
    case 0x0432: return QLatin1Char('b');   // в
    case 0x0435: return QLatin1Char('e');   // е
    case 0x043A: return QLatin1Char('k');   // к
    case 0x043C: return QLatin1Char('m');   // м
    case 0x043D: return QLatin1Char('h');   // н
    case 0x043E: return QLatin1Char('o');   // о
    case 0x0440: return QLatin1Char('p');   // р
    case 0x0441: return QLatin1Char('c');   // с
    case 0x0442: return QLatin1Char('t');   // т
    case 0x0443: return QLatin1Char('y');   // у
    case 0x0445: return QLatin1Char('x');   // х
    default: return ch;
    }
}

QString Fmt::normalize(const QString& text) {
    QString s = text.toCaseFolded().simplified();   // регистр и лишние пробелы
    for (QChar& ch : s)
        ch = foldChar(ch);
    return s;
}

bool Fmt::matches(const QStringList& fields, const QString& needle) {
    const QString n = normalize(needle);
    if (n.isEmpty())
        return true;

    const QString haystack = normalize(fields.join(QLatin1Char(' ')));
    QString digits;                                  // только цифры — для поиска телефона без пробелов и дефисов
    for (QChar ch : haystack)
        if (ch.isDigit()) digits += ch;

    const QStringList tokens = n.split(QLatin1Char(' '));
    for (const QString& token : tokens) {
        if (token.isEmpty())
            continue;
        bool found = haystack.contains(token);
        if (!found) {
            bool allDigits = true;
            for (QChar ch : token)
                if (!ch.isDigit()) { allDigits = false; break; }
            found = allDigits && digits.contains(token);
        }
        if (!found)
            return false;                            // каждое слово запроса должно быть найдено
    }
    return true;
}

const char* Fmt::phoneMask() { return "+7 000 000-00-00;_"; }
