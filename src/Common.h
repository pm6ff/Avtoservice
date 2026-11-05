#pragma once
#include <QString>
#include <QStringList>

namespace Role {
inline const QString Admin     = QStringLiteral("Администратор");
inline const QString Reception = QStringLiteral("Приёмщик");
inline const QString Master    = QStringLiteral("Мастер");
QStringList all();
}

namespace Status {
inline const QString Accepted = QStringLiteral("Принят");
inline const QString InWork   = QStringLiteral("В работе");
inline const QString Ready    = QStringLiteral("Готов");
inline const QString Issued   = QStringLiteral("Выдан");
QStringList all();
}

namespace Staff {
inline const QString Working = QStringLiteral("Работает");
inline const QString DayOff  = QStringLiteral("Выходной");
QStringList all();
}

namespace Kind {
inline const QString Service = QStringLiteral("Услуга");
inline const QString Part    = QStringLiteral("Запчасть");
QStringList all();
}

struct User {
    int id = 0;
    QString fullName, role, phone, status, login;
};

class Session {
public:
    static Session& instance();
    User user;
    bool isAdmin() const { return user.role == Role::Admin; }
    bool isMaster() const { return user.role == Role::Master; }
    void clear() { user = User(); }
};

namespace Fmt {
QString money(double value);                       // 8 500 ₽
QString shortName(const QString& fullName);        // Иванов Иван Иванович -> Иванов И.И.
QString dateUi(const QString& iso);                // 2026-10-25 -> 25.10.2026
QString normalize(const QString& text);            // нижний регистр, один пробел, ё=е, кириллица≈латиница
// Поиск: все слова запроса должны встретиться в полях (в любом порядке, без учёта регистра).
bool matches(const QStringList& fields, const QString& needle);
const char* phoneMask();                           // +7 000 000-00-00;_
}
