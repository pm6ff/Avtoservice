#include "RolesPage.h"
#include "Common.h"
#include "Database.h"
#include "Style.h"
#include <QAction>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

static QString accessFor(const QString& role) {
    if (role == Role::Master) return QStringLiteral("Мои заказ-наряды, История работ");
    if (role == Role::Reception) return QStringLiteral("Заказ-наряды, Клиенты, Автомобили, Услуги и запчасти");
    return QStringLiteral("Все разделы");
}

RolesPage::RolesPage(QWidget* parent) : Page(parent) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(36, 32, 36, 36);
    v->setSpacing(14);

    v->addWidget(Style::label(QStringLiteral("Пользователи и назначенные роли"), "h2"));
    m_table = Style::table({QStringLiteral("ФИО"), QStringLiteral("Текущая роль"),
                            QStringLiteral("Доступ к разделам"), QStringLiteral("Статус")});
    m_table->setMaximumWidth(1000);
    auto* tr = new QHBoxLayout;
    tr->addWidget(m_table, 1);
    tr->addStretch(1);
    v->addLayout(tr);

    v->addSpacing(8);
    v->addWidget(Style::label(QStringLiteral("Описание ролей"), "h2"));
    auto* cards = new QHBoxLayout;
    cards->setSpacing(20);
    const QList<QPair<QString, QString>> info = {
        {Role::Admin, QStringLiteral("Полный доступ ко всем разделам. Управление сотрудниками, ролями, заказами и отчётами.")},
        {Role::Reception, QStringLiteral("Работа с клиентами, автомобилями и заказ-нарядами. Добавление работ и запчастей, расчёт стоимости.")},
        {Role::Master, QStringLiteral("Просмотр назначенных заказ-нарядов. Работа с ремонтом и статусами выполнения.")}};
    for (const auto& p : info) {
        QFrame* c = Style::card();
        c->setMinimumHeight(150);
        auto* cv = new QVBoxLayout(c);
        cv->setContentsMargins(18, 16, 18, 16);
        cv->addWidget(Style::label(p.first, "h3"));
        auto* t = Style::label(p.second, "hint");
        t->setWordWrap(true);
        t->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        cv->addWidget(t, 1);
        cards->addWidget(c, 1);
    }
    v->addLayout(cards);
    v->addStretch(1);
}

void RolesPage::refresh() {
    const auto rows = Database::instance().select(
        QStringLiteral("SELECT id, full_name, role FROM employees ORDER BY id"));
    m_table->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        const int id = rows[i][0].toInt();
        const QString role = rows[i][2].toString();
        Style::setText(m_table, i, 0, rows[i][1].toString(), id);
        Style::setText(m_table, i, 1, role);
        Style::setText(m_table, i, 2, accessFor(role));

        auto* btn = new QPushButton(QStringLiteral("Изменить роль"));
        auto* menu = new QMenu(btn);
        for (const QString& r : Role::all()) {
            if (r == role) continue;
            QAction* a = menu->addAction(r);
            connect(a, &QAction::triggered, this, [this, id, r] { changeRole(id, r); });
        }
        btn->setMenu(menu);
        m_table->setCellWidget(i, 3, btn);
    }
    m_table->verticalHeader()->setDefaultSectionSize(56);
    Style::fitTable(m_table, 420);
}

void RolesPage::changeRole(int id, const QString& newRole) {
    if (!Session::instance().isAdmin()) return;
    auto& db = Database::instance();
    auto warn = [this](const QString& t) { Msg::warn(this, QStringLiteral("Изменение роли"), t); };

    if (id == Session::instance().user.id)
        return warn(QStringLiteral("Нельзя изменить собственную роль."));
    const QString old = db.scalar(QStringLiteral("SELECT role FROM employees WHERE id = ?"), {id}).toString();
    if (old == Role::Admin && db.scalar(QStringLiteral("SELECT COUNT(*) FROM employees WHERE role = ?"), {Role::Admin}).toInt() <= 1)
        return warn(QStringLiteral("Нельзя снять роль у последнего администратора."));
    if (old == Role::Master && newRole != Role::Master &&
        db.scalar(QStringLiteral("SELECT COUNT(*) FROM orders WHERE master_id = ? AND status IN (?, ?)"),
                  {id, Status::Accepted, Status::InWork}).toInt() > 0)
        return warn(QStringLiteral("У мастера есть незавершённые заказ-наряды. Сначала завершите их."));
    if (!Msg::confirm(this, QStringLiteral("Изменение роли"), QStringLiteral("Назначить роль «%1»?").arg(newRole)))
        return;

    QString err;
    if (db.exec(QStringLiteral("UPDATE employees SET role = ? WHERE id = ?"), {newRole, id}, &err) < 0)
        Msg::warn(this, QStringLiteral("Ошибка"), err);
    QTimer::singleShot(0, this, [this] { refresh(); });
}
