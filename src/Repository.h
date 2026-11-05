#pragma once
#include <QList>
#include <QString>

struct OrderRow {
    int id = 0;
    QString number, dateIso, client, clientPhone, car, plate, master, description, status;
    int masterId = 0;
    double total = 0;   // работы + запчасти, считается автоматически
};

namespace Repo {
QList<OrderRow> orders(int masterId = 0);   // masterId > 0 — только наряды мастера
QString nextOrderNumber();                  // ЗН-001, ЗН-002, ...
}
