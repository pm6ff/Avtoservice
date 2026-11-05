#include "OrdersPage.h"
#include "Common.h"
#include "Database.h"
#include "Pager.h"
#include "Repository.h"
#include "Style.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

OrdersPage::OrdersPage(QWidget* parent) : Page(parent) {
    m_admin = Session::instance().isAdmin();

    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(36, 28, 36, 36);
    v->setSpacing(12);

    // ----- верхняя строка: поиск + кнопки -----
    auto* top = new QHBoxLayout;
    auto* sv = new QVBoxLayout;
    sv->setSpacing(6);
    sv->addWidget(Style::label(QStringLiteral("Поиск"), "fieldLabel"));
    m_search = new QLineEdit;
    m_search->setPlaceholderText(QStringLiteral("Номер, клиент или автомобиль"));
    m_search->setMinimumWidth(480);
    sv->addWidget(m_search);
    top->addLayout(sv);
    top->addStretch(1);
    if (!m_admin) {
        auto* del = Style::button(QStringLiteral("Удалить"), "danger");
        top->addWidget(del, 0, Qt::AlignBottom);
        connect(del, &QPushButton::clicked, this, [this] {
            const int id = Style::currentId(m_table);
            if (id < 0) {
                Msg::info(this, QStringLiteral("Удаление"), QStringLiteral("Выберите заказ-наряд в таблице."));
                return;
            }
            deleteOrder(id);
        });
    }
    auto* add = Style::button(QStringLiteral("+ Добавить заказ-наряд"), "primary");
    add->setMinimumWidth(230);
    top->addWidget(add, 0, Qt::AlignBottom);
    v->addLayout(top);
    v->addSpacing(6);

    // ----- таблица -----
    QStringList headers = {QStringLiteral("№ заказа"), QStringLiteral("Дата"), QStringLiteral("Клиент"),
                           QStringLiteral("Автомобиль")};
    if (m_admin) headers << QStringLiteral("Мастер");
    headers << QStringLiteral("Описание неисправности") << QStringLiteral("Статус") << QStringLiteral("Сумма");
    if (m_admin) headers << QString();
    m_table = Style::table(headers);
    v->addWidget(m_table);
    m_pager = new Pager(8);          // по 8 заказ-нарядов на странице
    v->addWidget(m_pager);

    // ----- панель подробностей -----
    m_panel = Style::card();
    m_panel->hide();
    auto* pv = new QVBoxLayout(m_panel);
    pv->setContentsMargins(22, 20, 22, 20);
    pv->setSpacing(14);
    m_panelTitle = Style::label(QString(), "h2");
    pv->addWidget(m_panelTitle);

    auto* cols = new QHBoxLayout;
    cols->setSpacing(22);

    m_info = Style::table({QStringLiteral("Основная информация"), QString()});
    m_info->setRowCount(6);
    m_info->setFixedWidth(330);
    m_info->setSelectionMode(QAbstractItemView::NoSelection);
    cols->addWidget(m_info, 0, Qt::AlignTop);

    auto* mid = new QVBoxLayout;
    mid->setSpacing(14);
    m_works = Style::table({QStringLiteral("Наименование"), QStringLiteral("Нормо-часы"), QStringLiteral("Цена")});
    m_parts = Style::table({QStringLiteral("Наименование"), QStringLiteral("Количество"), QStringLiteral("Цена за ед.")});
    QPushButton *addW, *delW, *addP, *delP;
    mid->addWidget(section(QStringLiteral("Ремонтные работы"), m_works, &addW, &delW));
    mid->addWidget(section(QStringLiteral("Запчасти"), m_parts, &addP, &delP));
    cols->addLayout(mid, 1);

    auto* st = Style::card();
    st->setFixedWidth(240);
    auto* stv = new QVBoxLayout(st);
    stv->setContentsMargins(16, 16, 16, 16);
    stv->setSpacing(12);
    stv->addWidget(Style::label(QStringLiteral("Статус заказа"), "h3"));
    m_status = new QComboBox;
    m_status->addItems(Status::all());
    stv->addWidget(m_status);
    auto* save = Style::button(QStringLiteral("Сохранить изменения"), "primary");
    stv->addWidget(save);
    auto* del = Style::button(QStringLiteral("Удалить заказ"), "dangerSoft");
    stv->addWidget(del);
    stv->addStretch(1);
    cols->addWidget(st, 0, Qt::AlignTop);
    pv->addLayout(cols);

    m_total = Style::label(QString(), "h3");
    m_total->setAlignment(Qt::AlignRight);
    pv->addWidget(m_total);
    v->addWidget(m_panel);
    v->addStretch(1);

    // ----- сигналы -----
    connect(add, &QPushButton::clicked, this, [this] { emit navigate(QStringLiteral("order_form"), 0); });
    connect(m_search, &QLineEdit::textChanged, this, [this] {
        m_pager->reset();            // новый поиск — снова с первой страницы
        fillTable();
    });
    connect(m_pager, &Pager::pageChanged, this, [this] { fillTable(); });
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (auto* it = m_table->item(row, 0)) openOrder(it->data(Qt::UserRole).toInt());
    });
    connect(addW, &QPushButton::clicked, this, &OrdersPage::addWork);
    connect(delW, &QPushButton::clicked, this, &OrdersPage::removeWork);
    connect(addP, &QPushButton::clicked, this, &OrdersPage::addPart);
    connect(delP, &QPushButton::clicked, this, &OrdersPage::removePart);
    connect(save, &QPushButton::clicked, this, [this] {
        if (m_openId > 0) setStatus(m_openId, m_status->currentText());
    });
    connect(del, &QPushButton::clicked, this, [this] { if (m_openId > 0) deleteOrder(m_openId); });
}

QWidget* OrdersPage::section(const QString& title, QTableWidget* table, QPushButton** add, QPushButton** del) {
    auto* w = new QWidget;
    auto* v = new QVBoxLayout(w);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(6);
    auto* h = new QHBoxLayout;
    h->addWidget(Style::label(title, "h3"));
    h->addStretch(1);
    *add = Style::button(QStringLiteral("+ Добавить"), "secondary");
    *del = Style::button(QStringLiteral("Убрать"), "secondary");
    h->addWidget(*add);
    h->addWidget(*del);
    v->addLayout(h);
    v->addWidget(table);
    return w;
}

void OrdersPage::refresh() {
    fillTable();
    if (m_openId > 0) openOrder(m_openId);
}

void OrdersPage::fillTable() {
    const QList<OrderRow> rows = Repo::orders();
    const QString needle = m_search->text();
    QList<OrderRow> filtered;                         // сначала поиск, потом разбиение на страницы
    for (const OrderRow& r : rows)
        if (Fmt::matches({r.number, r.client, r.car, r.plate, r.description}, needle))
            filtered << r;
    m_pager->setTotal(int(filtered.size()));

    m_table->setRowCount(0);
    int row = 0;
    for (int i = m_pager->first(); i < m_pager->last(); ++i) {
        const OrderRow& r = filtered[i];
        m_table->insertRow(row);
        int c = 0;
        Style::setText(m_table, row, c++, r.number, r.id);
        Style::setText(m_table, row, c++, Fmt::dateUi(r.dateIso));
        Style::setText(m_table, row, c++, Fmt::shortName(r.client));
        Style::setText(m_table, row, c++, r.car);
        if (m_admin) Style::setText(m_table, row, c++, Fmt::shortName(r.master));
        Style::setText(m_table, row, c++, r.description);
        if (m_admin) {
            Style::setText(m_table, row, c++, r.status);
        } else {
            auto* cb = new QComboBox;
            cb->addItems(Status::all());
            cb->setCurrentText(r.status);
            const int id = r.id;
            connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [this, id, cb] { setStatus(id, cb->currentText()); });
            m_table->setCellWidget(row, c++, cb);
        }
        Style::setText(m_table, row, c++, Fmt::money(r.total));
        if (m_admin) {
            auto* open = Style::button(QStringLiteral("Открыть"), "secondary");
            const int id = r.id;
            connect(open, &QPushButton::clicked, this, [this, id] { openOrder(id); });
            m_table->setCellWidget(row, c++, open);
        }
        ++row;
    }
    Style::fitTable(m_table, m_panel->isVisible() ? 250 : 480);
}

void OrdersPage::closePanel() {
    m_openId = 0;
    m_panel->hide();
    Style::fitTable(m_table, 480);
}

void OrdersPage::openOrder(int id) {
    auto& db = Database::instance();
    const auto rows = db.select(
        QStringLiteral("SELECT o.number, o.created, c.full_name, c.phone, car.model, car.plate, e.full_name, o.status "
                       "FROM orders o JOIN clients c ON c.id = o.client_id JOIN cars car ON car.id = o.car_id "
                       "JOIN employees e ON e.id = o.master_id WHERE o.id = ?"),
        {id});
    if (rows.isEmpty()) {
        closePanel();
        return;
    }
    m_openId = id;
    const QVariantList& r = rows.first();
    m_panelTitle->setText(QStringLiteral("Заказ - наряд № ") + r[0].toString());

    const QStringList keys = {QStringLiteral("Клиент"), QStringLiteral("Телефон"), QStringLiteral("Автомобиль"),
                              QStringLiteral("Гос. номер"), QStringLiteral("Дата создания"), QStringLiteral("Мастер")};
    const QStringList vals = {Fmt::shortName(r[2].toString()), r[3].toString(), r[4].toString(), r[5].toString(),
                              Fmt::dateUi(r[1].toString()), Fmt::shortName(r[6].toString())};
    for (int i = 0; i < keys.size(); ++i) {
        Style::setText(m_info, i, 0, keys[i]);
        Style::setText(m_info, i, 1, vals[i]);
    }
    Style::fitTable(m_info, 400);
    m_status->setCurrentText(r[7].toString());

    const auto works = db.select(QStringLiteral("SELECT id, name, norm_hours, price FROM order_works WHERE order_id = ? ORDER BY id"), {id});
    m_works->setRowCount(works.size());
    for (int i = 0; i < works.size(); ++i) {
        Style::setText(m_works, i, 0, works[i][1].toString(), works[i][0].toInt());
        Style::setText(m_works, i, 1, QString::number(works[i][2].toDouble(), 'f', 1));
        Style::setText(m_works, i, 2, Fmt::money(works[i][3].toDouble()));
    }
    Style::fitTable(m_works, 200);

    const auto parts = db.select(QStringLiteral("SELECT id, name, qty, price FROM order_parts WHERE order_id = ? ORDER BY id"), {id});
    m_parts->setRowCount(parts.size());
    for (int i = 0; i < parts.size(); ++i) {
        Style::setText(m_parts, i, 0, parts[i][1].toString(), parts[i][0].toInt());
        Style::setText(m_parts, i, 1, QStringLiteral("%1 шт.").arg(parts[i][2].toInt()));
        Style::setText(m_parts, i, 2, Fmt::money(parts[i][3].toDouble()));
    }
    Style::fitTable(m_parts, 200);

    const double total = db.scalar(
        QStringLiteral("SELECT COALESCE((SELECT SUM(price) FROM order_works WHERE order_id = ?), 0) + "
                       "COALESCE((SELECT SUM(qty * price) FROM order_parts WHERE order_id = ?), 0)"),
        {id, id}).toDouble();
    m_total->setText(QStringLiteral("Итого к оплате: ") + Fmt::money(total));

    const bool wasHidden = m_panel->isHidden();
    m_panel->show();
    if (wasHidden) fillTable();   // уменьшаем высоту списка, чтобы панель была видна
    else Style::fitTable(m_table, 250);
}

void OrdersPage::setStatus(int id, const QString& status) {
    QString err;
    if (Database::instance().exec(QStringLiteral("UPDATE orders SET status = ? WHERE id = ?"), {status, id}, &err) < 0) {
        Msg::warn(this, QStringLiteral("Ошибка"), QStringLiteral("Не удалось сохранить статус:\n") + err);
        return;
    }
    // обновление списка — после выхода из обработчика сигнала (виджет-источник будет пересоздан)
    QTimer::singleShot(0, this, [this, id] {
        fillTable();
        if (m_openId == id) openOrder(id);
    });
}

void OrdersPage::deleteOrder(int id) {
    if (!Msg::confirm(this, QStringLiteral("Удаление заказ-наряда"),
                      QStringLiteral("Удалить заказ-наряд вместе с работами и запчастями?")))
        return;
    QString err;
    if (Database::instance().exec(QStringLiteral("DELETE FROM orders WHERE id = ?"), {id}, &err) < 0) {
        Msg::warn(this, QStringLiteral("Ошибка"), QStringLiteral("Не удалось удалить заказ-наряд:\n") + err);
        return;
    }
    if (m_openId == id) closePanel();
    QTimer::singleShot(0, this, [this] { fillTable(); });
}

void OrdersPage::addWork() {
    if (m_openId <= 0) return;
    auto& db = Database::instance();
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Добавить работу"));
    dlg.setMinimumWidth(460);
    auto* form = new QFormLayout(&dlg);

    auto* cb = new QComboBox;
    for (const QVariantList& s : db.select(QStringLiteral("SELECT name, price FROM catalog WHERE kind = ? ORDER BY name"), {Kind::Service}))
        cb->addItem(s[0].toString(), s[1].toDouble());
    auto* hours = new QDoubleSpinBox;
    hours->setRange(0.1, 100);
    hours->setDecimals(1);
    hours->setSingleStep(0.5);
    hours->setValue(1.0);
    auto* price = new QDoubleSpinBox;
    price->setRange(0, 9999999);
    price->setDecimals(2);
    price->setSuffix(QStringLiteral(" ₽"));
    auto sync = [cb, price] { price->setValue(cb->currentData().toDouble()); };
    connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), &dlg, sync);
    sync();
    form->addRow(QStringLiteral("Работа"), cb);
    form->addRow(QStringLiteral("Нормо-часы"), hours);
    form->addRow(QStringLiteral("Цена"), price);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    bb->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Добавить"));
    bb->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Отмена"));
    form->addRow(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (cb->count() == 0) {
        Msg::info(this, QStringLiteral("Справочник пуст"), QStringLiteral("Сначала добавьте услуги в справочник «Услуги и запчасти»."));
        return;
    }
    if (dlg.exec() != QDialog::Accepted) return;

    QString err;
    if (db.exec(QStringLiteral("INSERT INTO order_works(order_id, name, norm_hours, price) VALUES (?,?,?,?)"),
                {m_openId, cb->currentText(), hours->value(), price->value()}, &err) < 0)
        Msg::warn(this, QStringLiteral("Ошибка"), err);
    refresh();
}

void OrdersPage::addPart() {
    if (m_openId <= 0) return;
    auto& db = Database::instance();
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Добавить запчасть"));
    dlg.setMinimumWidth(460);
    auto* form = new QFormLayout(&dlg);

    auto* cb = new QComboBox;
    for (const QVariantList& s : db.select(QStringLiteral("SELECT name, price FROM catalog WHERE kind = ? ORDER BY name"), {Kind::Part}))
        cb->addItem(s[0].toString(), s[1].toDouble());
    auto* qty = new QSpinBox;
    qty->setRange(1, 999);
    auto* price = new QDoubleSpinBox;
    price->setRange(0, 9999999);
    price->setDecimals(2);
    price->setSuffix(QStringLiteral(" ₽"));
    auto sync = [cb, price] { price->setValue(cb->currentData().toDouble()); };
    connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), &dlg, sync);
    sync();
    form->addRow(QStringLiteral("Запчасть"), cb);
    form->addRow(QStringLiteral("Количество"), qty);
    form->addRow(QStringLiteral("Цена за единицу"), price);

    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    bb->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Добавить"));
    bb->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Отмена"));
    form->addRow(bb);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (cb->count() == 0) {
        Msg::info(this, QStringLiteral("Справочник пуст"), QStringLiteral("Сначала добавьте запчасти в справочник «Услуги и запчасти»."));
        return;
    }
    if (dlg.exec() != QDialog::Accepted) return;

    QString err;
    if (db.exec(QStringLiteral("INSERT INTO order_parts(order_id, name, qty, price) VALUES (?,?,?,?)"),
                {m_openId, cb->currentText(), qty->value(), price->value()}, &err) < 0)
        Msg::warn(this, QStringLiteral("Ошибка"), err);
    refresh();
}

void OrdersPage::removeWork() {
    const int id = Style::currentId(m_works);
    if (id < 0 || m_openId <= 0) return;
    Database::instance().exec(QStringLiteral("DELETE FROM order_works WHERE id = ? AND order_id = ?"), {id, m_openId});
    refresh();
}

void OrdersPage::removePart() {
    const int id = Style::currentId(m_parts);
    if (id < 0 || m_openId <= 0) return;
    Database::instance().exec(QStringLiteral("DELETE FROM order_parts WHERE id = ? AND order_id = ?"), {id, m_openId});
    refresh();
}
