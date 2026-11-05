#include "MasterOrdersPage.h"
#include "Common.h"
#include "Database.h"
#include "Pager.h"
#include "Repository.h"
#include "Style.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

MasterOrdersPage::MasterOrdersPage(Mode mode, QWidget* parent) : Page(parent), m_mode(mode) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(36, 32, 36, 36);
    v->setSpacing(10);

    const QString heading = mode == Home ? QStringLiteral("Мои задачи на сегодня")
                          : mode == Mine ? QStringLiteral("Заказ-наряды, назначенные мне")
                                         : QStringLiteral("Выполненные мной работы");
    v->addWidget(Style::label(heading, "fieldLabel"));
    m_search = new QLineEdit;
    m_search->setPlaceholderText(QStringLiteral("Номер заказа или автомобиль"));
    m_search->setFixedWidth(500);
    v->addWidget(m_search, 0, Qt::AlignLeft);
    v->addSpacing(10);

    m_table = Style::table({QStringLiteral("№ заказа"), QStringLiteral("Дата"), QStringLiteral("Клиент"),
                            QStringLiteral("Автомобиль"), QStringLiteral("Работа"),
                            QStringLiteral("Статус"), QStringLiteral("Сумма")});
    v->addWidget(m_table);
    m_pager = new Pager(8);          // по 8 заказ-нарядов на странице
    v->addWidget(m_pager);

    if (mode == Home) {
        auto* all = Style::button(QStringLiteral("Все заказ-наряды"), "primary");
        all->setMinimumWidth(230);
        v->addSpacing(8);
        v->addWidget(all, 0, Qt::AlignLeft);
        connect(all, &QPushButton::clicked, this, [this] { emit navigate(QStringLiteral("my_orders")); });
    }
    v->addStretch(1);

    connect(m_search, &QLineEdit::textChanged, this, [this] {
        m_pager->reset();            // новый поиск — снова с первой страницы
        refresh();
    });
    connect(m_pager, &Pager::pageChanged, this, [this] { refresh(); });
}

void MasterOrdersPage::refresh() {
    const int me = Session::instance().user.id;
    const QList<OrderRow> all = Repo::orders(me);   // только наряды текущего мастера
    const bool history = m_mode == History;

    QList<OrderRow> filtered;                         // сначала фильтр и поиск, потом страницы
    for (const OrderRow& r : all) {
        const bool finished = r.status == Status::Ready || r.status == Status::Issued;
        if (history != finished)
            continue;
        if (!Fmt::matches({r.number, r.car, r.plate, r.client, r.description}, m_search->text()))
            continue;
        filtered << r;
    }
    m_pager->setTotal(int(filtered.size()));

    m_table->setRowCount(0);
    int row = 0;
    for (int i = m_pager->first(); i < m_pager->last(); ++i) {
        const OrderRow& r = filtered[i];
        m_table->insertRow(row);
        Style::setText(m_table, row, 0, r.number, r.id);
        Style::setText(m_table, row, 1, Fmt::dateUi(r.dateIso));
        Style::setText(m_table, row, 2, Fmt::shortName(r.client));
        Style::setText(m_table, row, 3, r.car);
        Style::setText(m_table, row, 4, r.description);
        if (history) {
            Style::setText(m_table, row, 5, r.status);
        } else {
            // мастер может ставить «Принят / В работе / Готов»; «Выдан» — действие приёмщика
            auto* cb = new QComboBox;
            cb->addItems({Status::Accepted, Status::InWork, Status::Ready});
            cb->setCurrentText(r.status);
            const int id = r.id;
            connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                    [this, id, cb] { changeStatus(id, cb->currentText()); });
            m_table->setCellWidget(row, 5, cb);
        }
        Style::setText(m_table, row, 6, Fmt::money(r.total));
        ++row;
    }
    Style::fitTable(m_table, 460);
}

void MasterOrdersPage::changeStatus(int orderId, const QString& status) {
    QString err;
    // условие master_id = ? — мастер не может менять чужие наряды
    const int n = Database::instance().exec(
        QStringLiteral("UPDATE orders SET status = ? WHERE id = ? AND master_id = ?"),
        {status, orderId, Session::instance().user.id}, &err);
    if (n <= 0) {
        Msg::warn(this, QStringLiteral("Ошибка"), QStringLiteral("Не удалось изменить статус."));
    }
    // перерисовываем после выхода из обработчика сигнала комбобокса
    QTimer::singleShot(0, this, [this] { refresh(); });
}
