#include "Database.h"
#include <QCryptographicHash>
#include <QDate>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QPasswordDigestor>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUuid>

Database& Database::instance() {
    static Database db;
    return db;
}

bool Database::open(const QString& path, QString* error) {
    QDir().mkpath(QFileInfo(path).absolutePath());
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("autoservice"));
    m_db.setDatabaseName(path);
    if (!m_db.open()) {
        if (error) *error = m_db.lastError().text();
        return false;
    }
    exec(QStringLiteral("PRAGMA foreign_keys = ON"));
    if (!createSchema(error))
        return false;
    seedIfEmpty();
    return true;
}

QList<QVariantList> Database::select(const QString& sql, const QVariantList& args) {
    QList<QVariantList> out;
    QSqlQuery q(m_db);
    if (!q.prepare(sql)) {
        qWarning() << "SQL prepare:" << q.lastError().text();
        return out;
    }
    for (const QVariant& a : args) q.addBindValue(a);
    if (!q.exec()) {
        qWarning() << "SQL exec:" << q.lastError().text();
        return out;
    }
    const int n = q.record().count();
    while (q.next()) {
        QVariantList row;
        row.reserve(n);
        for (int i = 0; i < n; ++i) row << q.value(i);
        out << row;
    }
    return out;
}

QVariant Database::scalar(const QString& sql, const QVariantList& args) {
    const auto rows = select(sql, args);
    if (rows.isEmpty() || rows.first().isEmpty())
        return QVariant();
    return rows.first().first();
}

int Database::exec(const QString& sql, const QVariantList& args, QString* error, qint64* lastId) {
    QSqlQuery q(m_db);
    if (!q.prepare(sql)) {
        if (error) *error = q.lastError().text();
        return -1;
    }
    for (const QVariant& a : args) q.addBindValue(a);
    if (!q.exec()) {
        if (error) *error = q.lastError().text();
        return -1;
    }
    if (lastId) *lastId = q.lastInsertId().toLongLong();
    return q.numRowsAffected();
}

bool Database::transaction() { return m_db.transaction(); }
bool Database::commit() { return m_db.commit(); }
void Database::rollback() { m_db.rollback(); }

bool Database::isForeignKeyError(const QString& err) {
    return err.contains(QStringLiteral("FOREIGN KEY"), Qt::CaseInsensitive);
}
bool Database::isUniqueError(const QString& err) {
    return err.contains(QStringLiteral("UNIQUE"), Qt::CaseInsensitive);
}

// ---------- пароли: PBKDF2-HMAC-SHA256 + индивидуальная соль ----------
QString Database::newSalt() {
    return QUuid::createUuid().toString(QUuid::Id128);
}

QString Database::hashPassword(const QString& password, const QString& saltHex) {
    const QByteArray key = QPasswordDigestor::deriveKeyPbkdf2(
        QCryptographicHash::Sha256, password.toUtf8(), QByteArray::fromHex(saltHex.toLatin1()),
        120000, 32);
    return QString::fromLatin1(key.toHex());
}

static bool constantTimeEqual(const QByteArray& a, const QByteArray& b) {
    if (a.size() != b.size()) return false;
    int diff = 0;
    for (int i = 0; i < a.size(); ++i) diff |= (a[i] ^ b[i]);
    return diff == 0;
}

bool Database::authenticate(const QString& login, const QString& password, User* out) {
    const auto rows = select(
        QStringLiteral("SELECT id, full_name, role, phone, status, login, pass_hash, salt "
                       "FROM employees WHERE login = ?"),
        {login.trimmed()});
    // хеш считаем всегда, чтобы время ответа не выдавало существование логина
    const QString salt = rows.isEmpty() ? QStringLiteral("00") : rows.first()[7].toString();
    const QString hash = hashPassword(password, salt);
    if (rows.isEmpty())
        return false;
    const QVariantList& r = rows.first();
    if (!constantTimeEqual(hash.toUtf8(), r[6].toString().toUtf8()))
        return false;
    if (out) {
        out->id = r[0].toInt();
        out->fullName = r[1].toString();
        out->role = r[2].toString();
        out->phone = r[3].toString();
        out->status = r[4].toString();
        out->login = r[5].toString();
    }
    return true;
}

bool Database::setPassword(int employeeId, const QString& password) {
    const QString salt = newSalt();
    return exec(QStringLiteral("UPDATE employees SET pass_hash = ?, salt = ? WHERE id = ?"),
                {hashPassword(password, salt), salt, employeeId}) >= 0;
}

// ---------- схема ----------
bool Database::createSchema(QString* error) {
    const QStringList ddl = {
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS employees ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " full_name TEXT NOT NULL,"
            " role TEXT NOT NULL CHECK (role IN ('Администратор','Приёмщик','Мастер')),"
            " phone TEXT NOT NULL DEFAULT '',"
            " status TEXT NOT NULL DEFAULT 'Работает' CHECK (status IN ('Работает','Выходной')),"
            " login TEXT NOT NULL UNIQUE,"
            " pass_hash TEXT NOT NULL,"
            " salt TEXT NOT NULL)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS clients ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " full_name TEXT NOT NULL,"
            " phone TEXT NOT NULL,"
            " note TEXT NOT NULL DEFAULT '')"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS cars ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " client_id INTEGER NOT NULL REFERENCES clients(id) ON DELETE RESTRICT,"
            " model TEXT NOT NULL,"
            " plate TEXT NOT NULL UNIQUE,"
            " year INTEGER NOT NULL,"
            " vin TEXT NOT NULL UNIQUE)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS catalog ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " name TEXT NOT NULL,"
            " kind TEXT NOT NULL CHECK (kind IN ('Услуга','Запчасть')),"
            " price REAL NOT NULL CHECK (price >= 0),"
            " unit TEXT NOT NULL)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS orders ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " number TEXT NOT NULL UNIQUE,"
            " created TEXT NOT NULL,"
            " client_id INTEGER NOT NULL REFERENCES clients(id) ON DELETE RESTRICT,"
            " car_id INTEGER NOT NULL REFERENCES cars(id) ON DELETE RESTRICT,"
            " master_id INTEGER NOT NULL REFERENCES employees(id) ON DELETE RESTRICT,"
            " description TEXT NOT NULL,"
            " status TEXT NOT NULL CHECK (status IN ('Принят','В работе','Готов','Выдан')))"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS order_works ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " order_id INTEGER NOT NULL REFERENCES orders(id) ON DELETE CASCADE,"
            " name TEXT NOT NULL,"
            " norm_hours REAL NOT NULL DEFAULT 1,"
            " price REAL NOT NULL CHECK (price >= 0))"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS order_parts ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " order_id INTEGER NOT NULL REFERENCES orders(id) ON DELETE CASCADE,"
            " name TEXT NOT NULL,"
            " qty INTEGER NOT NULL CHECK (qty > 0),"
            " price REAL NOT NULL CHECK (price >= 0))"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_orders_master ON orders(master_id)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_orders_created ON orders(created)"),
        QStringLiteral("CREATE INDEX IF NOT EXISTS idx_cars_client ON cars(client_id)"),
    };
    for (const QString& s : ddl) {
        QString err;
        if (exec(s, {}, &err) < 0) {
            if (error) *error = err;
            return false;
        }
    }
    return true;
}

// ---------- демонстрационные данные (как на макетах) ----------
void Database::seedIfEmpty() {
    if (scalar(QStringLiteral("SELECT COUNT(*) FROM employees")).toInt() > 0)
        return;

    transaction();
    auto emp = [this](const QString& name, const QString& role, const QString& phone,
                      const QString& status, const QString& login, const QString& pw) {
        qint64 id = 0;
        const QString salt = newSalt();
        exec(QStringLiteral("INSERT INTO employees(full_name, role, phone, status, login, pass_hash, salt) "
                            "VALUES (?,?,?,?,?,?,?)"),
             {name, role, phone, status, login, hashPassword(pw, salt), salt}, nullptr, &id);
        return int(id);
    };
    const int kuznetsov = emp(QStringLiteral("Кузнецов Д.В."), Role::Master, QStringLiteral("+7 900 555-66-77"),
                              Staff::Working, QStringLiteral("master01"), QStringLiteral("master123"));
    emp(QStringLiteral("Волков Ю.М."), Role::Reception, QStringLiteral("+7 900 666-77-88"),
        Staff::Working, QStringLiteral("reception"), QStringLiteral("reception123"));
    const int morozov = emp(QStringLiteral("Морозов П.Г."), Role::Master, QStringLiteral("+7 900 777-88-99"),
                            Staff::Working, QStringLiteral("master02"), QStringLiteral("master123"));
    const int sokolov = emp(QStringLiteral("Соколов А.А."), Role::Master, QStringLiteral("+7 900 888-99-00"),
                            Staff::DayOff, QStringLiteral("master03"), QStringLiteral("master123"));
    emp(QStringLiteral("Администратор"), Role::Admin, QStringLiteral("+7 900 000-00-01"),
        Staff::Working, QStringLiteral("admin"), QStringLiteral("admin123"));

    auto client = [this](const QString& name, const QString& phone, const QString& note) {
        qint64 id = 0;
        exec(QStringLiteral("INSERT INTO clients(full_name, phone, note) VALUES (?,?,?)"),
             {name, phone, note}, nullptr, &id);
        return int(id);
    };
    const int c1 = client(QStringLiteral("Иванов Иван Иванович"), QStringLiteral("+7 900 111-22-33"), QStringLiteral("Постоянный клиент"));
    const int c2 = client(QStringLiteral("Петров Алексей Сергеевич"), QStringLiteral("+7 900 222-33-44"), QString());
    const int c3 = client(QStringLiteral("Сидоров Павел Викторович"), QStringLiteral("+7 900 333-44-55"), QString());
    const int c4 = client(QStringLiteral("Смирнов Андрей Андреевич"), QStringLiteral("+7 900 444-55-66"), QString());

    auto car = [this](int clientId, const QString& model, const QString& plate, int year, const QString& vin) {
        qint64 id = 0;
        exec(QStringLiteral("INSERT INTO cars(client_id, model, plate, year, vin) VALUES (?,?,?,?,?)"),
             {clientId, model, plate, year, vin}, nullptr, &id);
        return int(id);
    };
    const int a1 = car(c1, QStringLiteral("Ford Focus"), QStringLiteral("A123BC 77"), 2018, QStringLiteral("WF0XXX123"));
    const int a2 = car(c2, QStringLiteral("Kia Rio"), QStringLiteral("B456OP 77"), 2020, QStringLiteral("Z94CXX456"));
    const int a3 = car(c3, QStringLiteral("Lada Vesta"), QStringLiteral("E789KX 77"), 2021, QStringLiteral("XTA0XX789"));
    const int a4 = car(c4, QStringLiteral("Toyota Camry"), QStringLiteral("M111MM 77"), 2019, QStringLiteral("JTNBXX111"));

    auto cat = [this](const QString& name, const QString& kind, double price, const QString& unit) {
        exec(QStringLiteral("INSERT INTO catalog(name, kind, price, unit) VALUES (?,?,?,?)"),
             {name, kind, price, unit});
    };
    cat(QStringLiteral("Замена масла"), Kind::Service, 1500, QStringLiteral("работа"));
    cat(QStringLiteral("Диагностика двигателя"), Kind::Service, 2000, QStringLiteral("работа"));
    cat(QStringLiteral("Масляный фильтр"), Kind::Part, 650, QStringLiteral("шт."));
    cat(QStringLiteral("Тормозные колодки"), Kind::Part, 3200, QStringLiteral("комплект"));
    cat(QStringLiteral("Замена тормозных колодок"), Kind::Service, 2500, QStringLiteral("работа"));
    cat(QStringLiteral("Масло моторное 5W-30"), Kind::Part, 4500, QStringLiteral("шт."));

    auto order = [this](const QString& num, const QString& date, int cl, int ca, int master,
                        const QString& descr, const QString& status) {
        qint64 id = 0;
        exec(QStringLiteral("INSERT INTO orders(number, created, client_id, car_id, master_id, description, status) "
                            "VALUES (?,?,?,?,?,?,?)"),
             {num, date, cl, ca, master, descr, status}, nullptr, &id);
        return int(id);
    };
    auto work = [this](int orderId, const QString& name, double hours, double price) {
        exec(QStringLiteral("INSERT INTO order_works(order_id, name, norm_hours, price) VALUES (?,?,?,?)"),
             {orderId, name, hours, price});
    };
    const int o1 = order(QStringLiteral("ЗН-001"), QStringLiteral("2026-10-25"), c1, a1, kuznetsov, QStringLiteral("Замена масла"), Status::InWork);
    work(o1, QStringLiteral("Замена масла"), 1.0, 4000);
    exec(QStringLiteral("INSERT INTO order_parts(order_id, name, qty, price) VALUES (?,?,?,?)"),
         {o1, QStringLiteral("Масло моторное 5W-30"), 1, 4500});
    const int o2 = order(QStringLiteral("ЗН-002"), QStringLiteral("2026-10-24"), c2, a2, morozov, QStringLiteral("Диагностика"), Status::InWork);
    work(o2, QStringLiteral("Диагностика двигателя"), 2.0, 4200);
    const int o3 = order(QStringLiteral("ЗН-003"), QStringLiteral("2026-10-24"), c3, a3, morozov, QStringLiteral("Осмотр автомобиля"), Status::Accepted);
    work(o3, QStringLiteral("Осмотр автомобиля"), 1.0, 2000);
    const int o4 = order(QStringLiteral("ЗН-004"), QStringLiteral("2026-10-23"), c4, a4, sokolov, QStringLiteral("Ремонт"), Status::Ready);
    work(o4, QStringLiteral("Ремонт подвески"), 4.0, 12600);

    // ---- Дополнительные демонстрационные данные для проверки постраничной навигации ----
    // 30 клиентов (у каждого свой автомобиль) и 108 заказ-нарядов.
    // Заказы поровну делятся между тремя мастерами, а статусы чередуются, поэтому:
    //   админ и приёмщик -> 112 заказов (14 страниц, с «…» в навигации);
    //   каждый мастер    -> 18 активных и 18 выполненных (по 3 страницы в «Главная»/«Мои заказ-наряды»/«История»).
    const QStringList surnames = {
        QStringLiteral("Кузьмин"), QStringLiteral("Орлов"), QStringLiteral("Фёдоров"), QStringLiteral("Лебедев"),
        QStringLiteral("Новиков"), QStringLiteral("Захаров"), QStringLiteral("Беляев"), QStringLiteral("Макаров"),
        QStringLiteral("Зайцев"), QStringLiteral("Власов"), QStringLiteral("Григорьев"), QStringLiteral("Ершов"),
        QStringLiteral("Тихонов"), QStringLiteral("Комаров"), QStringLiteral("Дьячков")};
    const QStringList firstNames = {
        QStringLiteral("Сергей"), QStringLiteral("Дмитрий"), QStringLiteral("Михаил"), QStringLiteral("Николай"),
        QStringLiteral("Олег"), QStringLiteral("Артём"), QStringLiteral("Евгений"), QStringLiteral("Роман"),
        QStringLiteral("Виктор"), QStringLiteral("Максим")};
    const QStringList patronymics = {
        QStringLiteral("Александрович"), QStringLiteral("Игоревич"), QStringLiteral("Петрович"),
        QStringLiteral("Олегович"), QStringLiteral("Сергеевич"), QStringLiteral("Владимирович")};
    const QStringList models = {
        QStringLiteral("Ford Focus"), QStringLiteral("Kia Rio"), QStringLiteral("Lada Vesta"),
        QStringLiteral("Toyota Camry"), QStringLiteral("Hyundai Solaris"), QStringLiteral("Skoda Octavia"),
        QStringLiteral("Volkswagen Polo"), QStringLiteral("Renault Logan"), QStringLiteral("Nissan Qashqai"),
        QStringLiteral("Mazda 3")};
    const char plateLetters[] = "ABEKMHOPCTYX";

    QList<int> clientIds = {c1, c2, c3, c4};
    QList<int> carIds = {a1, a2, a3, a4};
    for (int i = 0; i < 30; ++i) {
        const QString name = QStringLiteral("%1 %2 %3")
                                 .arg(surnames[i % surnames.size()], firstNames[(i * 3) % firstNames.size()],
                                      patronymics[(i * 5) % patronymics.size()]);
        const QString phone = QStringLiteral("+7 91%1 %2-%3-%4")
                                  .arg(i % 10).arg(100 + i * 13, 3, 10, QLatin1Char('0'))
                                  .arg((i * 7) % 100, 2, 10, QLatin1Char('0'))
                                  .arg((i * 11 + 5) % 100, 2, 10, QLatin1Char('0'));
        const int cid = client(name, phone, i % 5 == 0 ? QStringLiteral("Постоянный клиент") : QString());
        const QString plate = QStringLiteral("%1%2%3%4 77")
                                  .arg(QLatin1Char(plateLetters[i % 12]))
                                  .arg(200 + i * 7)
                                  .arg(QLatin1Char(plateLetters[(i / 12) % 12]))
                                  .arg(QLatin1Char(plateLetters[(i / 3) % 12]));
        const int aid = car(cid, models[i % models.size()], plate, 2012 + i % 12,
                            QStringLiteral("VIN%1").arg(100000 + i));
        clientIds << cid;
        carIds << aid;
    }

    const int masterIds[] = {kuznetsov, morozov, sokolov};
    const QStringList problems = {
        QStringLiteral("Замена масла"), QStringLiteral("Диагностика"), QStringLiteral("Замена тормозных колодок"),
        QStringLiteral("Осмотр автомобиля"), QStringLiteral("Проверка подвески"), QStringLiteral("Замена фильтров")};
    const QStringList statuses = {Status::Accepted, Status::InWork, Status::Ready, Status::Issued};
    for (int i = 0; i < 108; ++i) {
        const int k = (i * 7) % int(clientIds.size());      // клиент (и его автомобиль)
        const int master = masterIds[i % 3];
        const QString status = statuses[(i / 3) % 4];       // у каждого мастера статусы идут по кругу
        const QString date = QDate(2026, 10, 22).addDays(-(i / 4)).toString(Qt::ISODate);
        const QString descr = problems[i % problems.size()];
        const int id = order(QStringLiteral("ЗН-%1").arg(5 + i, 3, 10, QLatin1Char('0')), date,
                             clientIds[k], carIds[k], master, descr, status);
        work(id, descr, 1.0, 1500 + 500 * (i % 5));
    }
    commit();
}
