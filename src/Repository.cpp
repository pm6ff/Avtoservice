#include "Repository.h"
#include "Database.h"

QList<OrderRow> Repo::orders(int masterId) {
    QString sql =
        "SELECT o.id, o.number, o.created, c.full_name, c.phone, car.model, car.plate,"
        " e.full_name, o.master_id, o.description, o.status,"
        " COALESCE((SELECT SUM(w.price) FROM order_works w WHERE w.order_id = o.id), 0) +"
        " COALESCE((SELECT SUM(p.qty * p.price) FROM order_parts p WHERE p.order_id = o.id), 0)"
        " FROM orders o"
        " JOIN clients c ON c.id = o.client_id"
        " JOIN cars car ON car.id = o.car_id"
        " JOIN employees e ON e.id = o.master_id ";
    QVariantList args;
    if (masterId > 0) {
        sql += "WHERE o.master_id = ? ";
        args << masterId;
    }
    sql += "ORDER BY o.created DESC, o.id DESC";

    QList<OrderRow> out;
    for (const QVariantList& r : Database::instance().select(sql, args)) {
        OrderRow o;
        o.id = r[0].toInt();
        o.number = r[1].toString();
        o.dateIso = r[2].toString();
        o.client = r[3].toString();
        o.clientPhone = r[4].toString();
        o.car = r[5].toString();
        o.plate = r[6].toString();
        o.master = r[7].toString();
        o.masterId = r[8].toInt();
        o.description = r[9].toString();
        o.status = r[10].toString();
        o.total = r[11].toDouble();
        out << o;
    }
    return out;
}

QString Repo::nextOrderNumber() {
    const int n = Database::instance()
                      .scalar(QStringLiteral("SELECT COALESCE(MAX(CAST(SUBSTR(number, 4) AS INTEGER)), 0) + 1 FROM orders"))
                      .toInt();
    return QStringLiteral("ЗН-%1").arg(n, 3, 10, QLatin1Char('0'));
}
