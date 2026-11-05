#include "EmployeesPage.h"
#include "Common.h"
#include "Database.h"
#include "Style.h"
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

EmployeesPage::EmployeesPage(QWidget* parent) : Page(parent) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(36, 32, 36, 36);
    v->setSpacing(12);

    auto* top = new QHBoxLayout;
    top->addWidget(Style::label(QStringLiteral("Список сотрудников"), "h2"));
    top->addStretch(1);
    auto* del = Style::button(QStringLiteral("Удалить"), "danger");
    auto* add = Style::button(QStringLiteral("+ Добавить сотрудника"), "primary");
    add->setMinimumWidth(220);
    top->addWidget(del);
    top->addWidget(add);
    v->addLayout(top);
    v->addSpacing(6);

    m_table = Style::table({QStringLiteral("ФИО"), QStringLiteral("Должность"),
                            QStringLiteral("Номер телефона"), QStringLiteral("Статус")});
    v->addWidget(m_table);
    v->addStretch(1);

    connect(add, &QPushButton::clicked, this, [this] { emit navigate(QStringLiteral("staff_form"), 0); });
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (auto* it = m_table->item(row, 0)) emit navigate(QStringLiteral("staff_form"), it->data(Qt::UserRole).toInt());
    });
    connect(del, &QPushButton::clicked, this, [this] {
        if (!Session::instance().isAdmin()) return;
        const int id = Style::currentId(m_table);
        if (id < 0) {
            Msg::info(this, QStringLiteral("Удаление"), QStringLiteral("Выберите сотрудника в таблице."));
            return;
        }
        auto& db = Database::instance();
        if (id == Session::instance().user.id) {
            Msg::warn(this, QStringLiteral("Удаление невозможно"), QStringLiteral("Нельзя удалить собственную учётную запись."));
            return;
        }
        const QString role = db.scalar(QStringLiteral("SELECT role FROM employees WHERE id = ?"), {id}).toString();
        if (role == Role::Admin && db.scalar(QStringLiteral("SELECT COUNT(*) FROM employees WHERE role = ?"), {Role::Admin}).toInt() <= 1) {
            Msg::warn(this, QStringLiteral("Удаление невозможно"), QStringLiteral("Нельзя удалить последнего администратора."));
            return;
        }
        if (!Msg::confirm(this, QStringLiteral("Удаление сотрудника"), QStringLiteral("Удалить выбранного сотрудника?")))
            return;
        QString err;
        if (db.exec(QStringLiteral("DELETE FROM employees WHERE id = ?"), {id}, &err) < 0) {
            if (Database::isForeignKeyError(err))
                Msg::warn(this, QStringLiteral("Удаление невозможно"),
                          QStringLiteral("За сотрудником закреплены заказ-наряды. Переведите его в статус «Выходной»."));
            else
                Msg::warn(this, QStringLiteral("Ошибка"), err);
        }
        QTimer::singleShot(0, this, [this] { refresh(); });
    });
}

void EmployeesPage::refresh() {
    const auto rows = Database::instance().select(
        QStringLiteral("SELECT id, full_name, role, phone, status FROM employees ORDER BY id"));
    m_table->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        const int id = rows[i][0].toInt();
        Style::setText(m_table, i, 0, rows[i][1].toString(), id);
        Style::setText(m_table, i, 1, rows[i][2].toString());
        Style::setText(m_table, i, 2, rows[i][3].toString());
        auto* cb = new QComboBox;
        cb->addItems(Staff::all());
        cb->setCurrentText(rows[i][4].toString());
        connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, id, cb] {
            if (!Session::instance().isAdmin()) return;
            Database::instance().exec(QStringLiteral("UPDATE employees SET status = ? WHERE id = ?"),
                                      {cb->currentText(), id});
        });
        m_table->setCellWidget(i, 3, cb);
    }
    Style::fitTable(m_table, 500);
}

// ===================== форма сотрудника =====================
EmployeeFormPage::EmployeeFormPage(QWidget* parent) : Page(parent) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(36, 32, 36, 36);
    v->setSpacing(10);
    v->addWidget(Style::label(QStringLiteral("Заполните данные для добавления нового сотрудника"), "hint"));
    v->addSpacing(8);

    m_name = new QLineEdit;
    m_name->setPlaceholderText(QStringLiteral("Введите ФИО сотрудника"));
    m_name->setMaxLength(120);
    m_role = new QComboBox;
    m_role->addItems(Role::all());
    m_phone = new QLineEdit;
    m_phone->setInputMask(QString::fromLatin1(Fmt::phoneMask()));
    m_status = new QComboBox;
    m_status->addItems(Staff::all());
    m_login = new QLineEdit;
    m_login->setPlaceholderText(QStringLiteral("Логин для входа"));
    m_login->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("[A-Za-z0-9_.-]{0,32}")), this));
    m_password = new QLineEdit;
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setMaxLength(128);

    auto* g = new QGridLayout;
    g->setHorizontalSpacing(40);
    g->setVerticalSpacing(8);
    g->setColumnMinimumWidth(0, 400);
    g->setColumnMinimumWidth(1, 400);
    g->addWidget(Style::label(QStringLiteral("ФИО"), "fieldLabel"), 0, 0);
    g->addWidget(Style::label(QStringLiteral("Должность"), "fieldLabel"), 0, 1);
    g->addWidget(m_name, 1, 0);
    g->addWidget(m_role, 1, 1);
    g->setRowMinimumHeight(2, 12);
    g->addWidget(Style::label(QStringLiteral("Телефон"), "fieldLabel"), 3, 0);
    g->addWidget(Style::label(QStringLiteral("Статус"), "fieldLabel"), 3, 1);
    g->addWidget(m_phone, 4, 0);
    g->addWidget(m_status, 4, 1);
    g->setRowMinimumHeight(5, 12);
    g->addWidget(Style::label(QStringLiteral("Логин"), "fieldLabel"), 6, 0);
    g->addWidget(Style::label(QStringLiteral("Пароль (не менее 8 символов)"), "fieldLabel"), 6, 1);
    g->addWidget(m_login, 7, 0);
    g->addWidget(m_password, 7, 1);
    g->setColumnStretch(2, 1);
    v->addLayout(g);
    v->addSpacing(22);

    auto* btns = new QHBoxLayout;
    auto* save = Style::button(QStringLiteral("Добавить сотрудника"), "primary");
    save->setMinimumWidth(210);
    auto* cancel = Style::button(QStringLiteral("Отмена"), "secondary");
    cancel->setMinimumWidth(130);
    btns->addWidget(save);
    btns->addWidget(cancel);
    btns->addStretch(1);
    v->addLayout(btns);
    v->addStretch(1);

    connect(save, &QPushButton::clicked, this, &EmployeeFormPage::save);
    connect(cancel, &QPushButton::clicked, this, [this] { emit navigate(QStringLiteral("staff")); });
}

void EmployeeFormPage::prepare(int id) {
    m_id = id;
    m_name->clear();
    m_phone->clear();
    m_login->clear();
    m_login->setEnabled(true);
    m_password->clear();
    m_password->setPlaceholderText(id > 0 ? QStringLiteral("Оставьте пустым, чтобы не менять")
                                          : QStringLiteral("Введите пароль"));
    m_role->setCurrentIndex(0);
    m_status->setCurrentIndex(0);
    if (id > 0) {
        const auto rows = Database::instance().select(
            QStringLiteral("SELECT full_name, role, phone, status, login FROM employees WHERE id = ?"), {id});
        if (!rows.isEmpty()) {
            m_name->setText(rows[0][0].toString());
            m_role->setCurrentText(rows[0][1].toString());
            m_phone->setText(rows[0][2].toString());
            m_status->setCurrentText(rows[0][3].toString());
            m_login->setText(rows[0][4].toString());
            m_login->setEnabled(false);
        }
    }
}

void EmployeeFormPage::save() {
    if (!Session::instance().isAdmin()) return;
    auto warn = [this](const QString& t) { Msg::warn(this, QStringLiteral("Проверьте данные"), t); };
    const QString name = m_name->text().simplified();
    const QString login = m_login->text().trimmed();
    const QString pw = m_password->text();

    if (name.size() < 3) return warn(QStringLiteral("Введите ФИО сотрудника."));
    if (!m_phone->hasAcceptableInput()) return warn(QStringLiteral("Введите номер телефона полностью."));
    if (login.size() < 3) return warn(QStringLiteral("Логин должен содержать не менее 3 символов."));
    if (m_id == 0 && pw.size() < 8) return warn(QStringLiteral("Пароль должен содержать не менее 8 символов."));
    if (m_id > 0 && !pw.isEmpty() && pw.size() < 8) return warn(QStringLiteral("Пароль должен содержать не менее 8 символов."));

    auto& db = Database::instance();
    QString err;
    if (m_id > 0) {
        const QString oldRole = db.scalar(QStringLiteral("SELECT role FROM employees WHERE id = ?"), {m_id}).toString();
        if (oldRole == Role::Admin && m_role->currentText() != Role::Admin &&
            db.scalar(QStringLiteral("SELECT COUNT(*) FROM employees WHERE role = ?"), {Role::Admin}).toInt() <= 1)
            return warn(QStringLiteral("Нельзя снять роль у последнего администратора."));
        if (db.exec(QStringLiteral("UPDATE employees SET full_name = ?, role = ?, phone = ?, status = ? WHERE id = ?"),
                    {name, m_role->currentText(), m_phone->text(), m_status->currentText(), m_id}, &err) < 0) {
            Msg::warn(this, QStringLiteral("Ошибка"), err);
            return;
        }
        if (!pw.isEmpty()) db.setPassword(m_id, pw);
    } else {
        const QString salt = Database::newSalt();
        if (db.exec(QStringLiteral("INSERT INTO employees(full_name, role, phone, status, login, pass_hash, salt) VALUES (?,?,?,?,?,?,?)"),
                    {name, m_role->currentText(), m_phone->text(), m_status->currentText(), login,
                     Database::hashPassword(pw, salt), salt}, &err) < 0) {
            if (Database::isUniqueError(err))
                warn(QStringLiteral("Сотрудник с таким логином уже существует."));
            else
                Msg::warn(this, QStringLiteral("Ошибка"), err);
            return;
        }
    }
    emit navigate(QStringLiteral("staff"));
}
