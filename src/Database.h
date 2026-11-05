#pragma once
#include "Common.h"
#include <QList>
#include <QSqlDatabase>
#include <QVariantList>

// Singleton: единственное подключение к БД (SQLite).
// Все запросы выполняются только как параметризованные.
class Database {
public:
    static Database& instance();

    bool open(const QString& path, QString* error = nullptr);

    QList<QVariantList> select(const QString& sql, const QVariantList& args = {});
    QVariant scalar(const QString& sql, const QVariantList& args = {});
    // Возвращает число затронутых строк или -1 при ошибке.
    int exec(const QString& sql, const QVariantList& args = {}, QString* error = nullptr,
             qint64* lastId = nullptr);

    bool transaction();
    bool commit();
    void rollback();

    bool authenticate(const QString& login, const QString& password, User* out);
    bool setPassword(int employeeId, const QString& password);

    static QString newSalt();
    static QString hashPassword(const QString& password, const QString& saltHex);
    static bool isForeignKeyError(const QString& err);
    static bool isUniqueError(const QString& err);

private:
    Database() = default;
    bool createSchema(QString* error);
    void seedIfEmpty();

    QSqlDatabase m_db;
};
