#include "OrderFormPage.h"
#include "Common.h"
#include "Database.h"
#include "Repository.h"
#include "Style.h"
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

OrderFormPage::OrderFormPage(QWidget* parent) : Page(parent) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(36, 32, 36, 36);
    v->setSpacing(10);
    v->addWidget(Style::label(QStringLiteral("Заполните данные нового заказ-наряда"), "hint"));
    v->addSpacing(8);

    auto* g = new QGridLayout;
    g->setHorizontalSpacing(40);
    g->setVerticalSpacing(8);
    g->setColumnMinimumWidth(0, 400);
    g->setColumnMinimumWidth(1, 400);

    m_client = new QComboBox;
    m_client->setPlaceholderText(QStringLiteral("Выберите клиента"));
    m_car = new QComboBox;
    m_car->setPlaceholderText(QStringLiteral("Выберите автомобиль"));
    m_date = new QDateEdit;
    m_date->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
    m_date->setCalendarPopup(true);
    m_master = new QComboBox;
    m_master->setPlaceholderText(QStringLiteral("Выберите мастера"));
    m_description = new QLineEdit;
    m_description->setPlaceholderText(QStringLiteral("Кратко опишите проблему"));
    m_description->setMaxLength(200);
    m_status = new QComboBox;
    m_status->addItems(Status::all());

    g->addWidget(Style::label(QStringLiteral("Клиент"), "fieldLabel"), 0, 0);
    g->addWidget(Style::label(QStringLiteral("Автомобиль"), "fieldLabel"), 0, 1);
    g->addWidget(m_client, 1, 0);
    g->addWidget(m_car, 1, 1);
    g->setRowMinimumHeight(2, 14);
    g->addWidget(Style::label(QStringLiteral("Дата приёма"), "fieldLabel"), 3, 0);
    g->addWidget(Style::label(QStringLiteral("Мастер"), "fieldLabel"), 3, 1);
    g->addWidget(m_date, 4, 0);
    g->addWidget(m_master, 4, 1);
    g->setRowMinimumHeight(5, 14);
    g->addWidget(Style::label(QStringLiteral("Описание неисправности"), "fieldLabel"), 6, 0, 1, 2);
    g->addWidget(m_description, 7, 0, 1, 2);
    g->setRowMinimumHeight(8, 14);
    g->addWidget(Style::label(QStringLiteral("Статус"), "fieldLabel"), 9, 0);
    g->addWidget(m_status, 10, 0);
    g->setColumnStretch(2, 1);
    v->addLayout(g);
    v->addSpacing(22);

    auto* btns = new QHBoxLayout;
    auto* save = Style::button(QStringLiteral("Сохранить заказ"), "primary");
    save->setMinimumWidth(190);
    auto* cancel = Style::button(QStringLiteral("Отмена"), "secondary");
    cancel->setMinimumWidth(130);
    btns->addWidget(save);
    btns->addWidget(cancel);
    btns->addStretch(1);
    v->addLayout(btns);
    v->addStretch(1);

    connect(m_client, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { loadCars(); });
    connect(save, &QPushButton::clicked, this, &OrderFormPage::save);
    connect(cancel, &QPushButton::clicked, this, [this] { emit navigate(QStringLiteral("orders")); });
}

void OrderFormPage::prepare(int) {
    auto& db = Database::instance();
    m_client->blockSignals(true);
    m_client->clear();
    for (const QVariantList& r : db.select(QStringLiteral("SELECT id, full_name FROM clients ORDER BY full_name")))
        m_client->addItem(r[1].toString(), r[0].toInt());
    m_client->setCurrentIndex(-1);
    m_client->blockSignals(false);
    loadCars();

    m_master->clear();
    for (const QVariantList& r : db.select(
             QStringLiteral("SELECT id, full_name FROM employees WHERE role = ? AND status = ? ORDER BY full_name"),
             {Role::Master, Staff::Working}))
        m_master->addItem(r[1].toString(), r[0].toInt());
    m_master->setCurrentIndex(-1);

    m_date->setDate(QDate::currentDate());
    m_description->clear();
    m_status->setCurrentText(Status::Accepted);
}

void OrderFormPage::loadCars() {
    m_car->clear();
    const int clientId = m_client->currentData().toInt();
    if (clientId > 0) {
        for (const QVariantList& r : Database::instance().select(
                 QStringLiteral("SELECT id, model, plate FROM cars WHERE client_id = ? ORDER BY model"), {clientId}))
            m_car->addItem(r[1].toString() + QStringLiteral(" (") + r[2].toString() + QStringLiteral(")"), r[0].toInt());
    }
    m_car->setCurrentIndex(-1);
}

void OrderFormPage::save() {
    auto warn = [this](const QString& t) { Msg::warn(this, QStringLiteral("Проверьте данные"), t); };
    if (m_client->currentIndex() < 0) return warn(QStringLiteral("Выберите клиента."));
    if (m_car->currentIndex() < 0) {
        return warn(m_car->count() == 0
                        ? QStringLiteral("У выбранного клиента нет автомобилей. Сначала добавьте автомобиль в разделе «Автомобили».")
                        : QStringLiteral("Выберите автомобиль."));
    }
    if (m_master->currentIndex() < 0) return warn(QStringLiteral("Выберите мастера."));
    const QString descr = m_description->text().trimmed();
    if (descr.isEmpty()) return warn(QStringLiteral("Опишите неисправность."));

    auto& db = Database::instance();
    db.transaction();
    QString err;
    const int n = db.exec(
        QStringLiteral("INSERT INTO orders(number, created, client_id, car_id, master_id, description, status) "
                       "VALUES (?,?,?,?,?,?,?)"),
        {Repo::nextOrderNumber(), m_date->date().toString(Qt::ISODate), m_client->currentData().toInt(),
         m_car->currentData().toInt(), m_master->currentData().toInt(), descr, m_status->currentText()}, &err);
    if (n < 0) {
        db.rollback();
        Msg::warn(this, QStringLiteral("Ошибка"), QStringLiteral("Не удалось сохранить заказ-наряд:\n") + err);
        return;
    }
    db.commit();
    emit navigate(QStringLiteral("orders"));
}
