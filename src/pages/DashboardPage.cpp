#include "DashboardPage.h"
#include "Common.h"
#include "Database.h"
#include "Repository.h"
#include "Style.h"
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QVBoxLayout>

DashboardPage::DashboardPage(QWidget* parent) : Page(parent) {
    m_admin = Session::instance().isAdmin();
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(36, 32, 36, 36);
    v->setSpacing(18);

    if (!m_admin)
        v->addWidget(Style::label(QStringLiteral("Рабочее место приёмщика"), "h2"));

    const QStringList captions = m_admin
        ? QStringList{QStringLiteral("Всего заказ-нарядов"), QStringLiteral("Автомобили в работе"),
                      QStringLiteral("Всего клиентов"), QStringLiteral("Сотрудники")}
        : QStringList{QStringLiteral("Открытые заказ-наряды"), QStringLiteral("Автомобили в работе"),
                      QStringLiteral("Ожидают выдачи")};
    const int cols = m_admin ? 2 : 3;

    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(30);
    grid->setVerticalSpacing(18);
    for (int i = 0; i < captions.size(); ++i) {
        QFrame* c = Style::card();
        c->setMinimumSize(300, 124);
        auto* cv = new QVBoxLayout(c);
        cv->setContentsMargins(20, 18, 20, 18);
        cv->addWidget(Style::label(captions[i], "statCaption"));
        cv->addStretch(1);
        QLabel* val = Style::label(QStringLiteral("0"), "statValue");
        cv->addWidget(val);
        m_values << val;
        grid->addWidget(c, i / cols, i % cols);
    }
    grid->setColumnStretch(cols, 1);
    v->addLayout(grid);

    v->addSpacing(6);
    v->addWidget(Style::label(QStringLiteral("Последние заказ-наряды"), "h2"));
    m_table = Style::table({QStringLiteral("№ заказа"), QStringLiteral("Клиент"),
                            QStringLiteral("Автомобиль"), QStringLiteral("Статус")});
    m_table->setMaximumWidth(900);
    auto* tableRow = new QHBoxLayout;
    tableRow->addWidget(m_table, 1);
    tableRow->addStretch(1);
    v->addLayout(tableRow);
    v->addStretch(1);
}

void DashboardPage::refresh() {
    auto& db = Database::instance();
    auto count = [&](const QString& sql, const QVariantList& a = {}) { return db.scalar(sql, a).toInt(); };

    if (m_admin) {
        m_values[0]->setText(QString::number(count(QStringLiteral("SELECT COUNT(*) FROM orders"))));
        m_values[1]->setText(QString::number(count(QStringLiteral("SELECT COUNT(*) FROM orders WHERE status = ?"), {Status::InWork})));
        m_values[2]->setText(QString::number(count(QStringLiteral("SELECT COUNT(*) FROM clients"))));
        m_values[3]->setText(QString::number(count(QStringLiteral("SELECT COUNT(*) FROM employees"))));
    } else {
        m_values[0]->setText(QString::number(count(QStringLiteral("SELECT COUNT(*) FROM orders WHERE status <> ?"), {Status::Issued})));
        m_values[1]->setText(QString::number(count(QStringLiteral("SELECT COUNT(*) FROM orders WHERE status = ?"), {Status::InWork})));
        m_values[2]->setText(QString::number(count(QStringLiteral("SELECT COUNT(*) FROM orders WHERE status = ?"), {Status::Ready})));
    }

    const QList<OrderRow> rows = Repo::orders();
    const int n = qMin(5, int(rows.size()));
    m_table->setRowCount(n);
    for (int i = 0; i < n; ++i) {
        const OrderRow& r = rows[i];
        Style::setText(m_table, i, 0, r.number, r.id);
        Style::setText(m_table, i, 1, Fmt::shortName(r.client));
        Style::setText(m_table, i, 2, r.car);
        Style::setText(m_table, i, 3, r.status);
    }
    Style::fitTable(m_table, 360);
}
