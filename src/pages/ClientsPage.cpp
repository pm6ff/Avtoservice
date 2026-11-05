#include "ClientsPage.h"
#include "Common.h"
#include "Database.h"
#include "Style.h"
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

ClientsPage::ClientsPage(QWidget* parent) : Page(parent) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(36, 28, 36, 36);
    v->setSpacing(12);

    auto* top = new QHBoxLayout;
    auto* sv = new QVBoxLayout;
    sv->setSpacing(6);
    sv->addWidget(Style::label(QStringLiteral("Поиск клиента"), "fieldLabel"));
    m_search = new QLineEdit;
    m_search->setPlaceholderText(QStringLiteral("ФИО или телефон"));
    m_search->setFixedWidth(500);
    sv->addWidget(m_search);
    top->addLayout(sv);
    top->addStretch(1);
    auto* del = Style::button(QStringLiteral("Удалить"), "danger");
    auto* add = Style::button(QStringLiteral("+ Добавить клиента"), "primary");
    add->setMinimumWidth(200);
    top->addWidget(del, 0, Qt::AlignBottom);
    top->addWidget(add, 0, Qt::AlignBottom);
    v->addLayout(top);
    v->addSpacing(6);

    m_table = Style::table({QStringLiteral("ФИО клиента"), QStringLiteral("Телефон"),
                            QStringLiteral("Количество заказов"), QStringLiteral("Примечание")});
    v->addWidget(m_table);
    v->addStretch(1);

    connect(m_search, &QLineEdit::textChanged, this, [this] { refresh(); });
    connect(add, &QPushButton::clicked, this, [this] { emit navigate(QStringLiteral("client_form"), 0); });
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (auto* it = m_table->item(row, 0)) emit navigate(QStringLiteral("client_form"), it->data(Qt::UserRole).toInt());
    });
    connect(del, &QPushButton::clicked, this, [this] {
        const int id = Style::currentId(m_table);
        if (id < 0) {
            Msg::info(this, QStringLiteral("Удаление"), QStringLiteral("Выберите клиента в таблице."));
            return;
        }
        auto& db = Database::instance();
        const QVariantList a{id};
        if (db.scalar(QStringLiteral("SELECT COUNT(*) FROM cars WHERE client_id = ?"), a).toInt() > 0 ||
            db.scalar(QStringLiteral("SELECT COUNT(*) FROM orders WHERE client_id = ?"), a).toInt() > 0) {
            Msg::warn(this, QStringLiteral("Удаление невозможно"),
                      QStringLiteral("У клиента есть автомобили или заказ-наряды. Сначала удалите их."));
            return;
        }
        if (!Msg::confirm(this, QStringLiteral("Удаление клиента"), QStringLiteral("Удалить выбранного клиента?")))
            return;
        QString err;
        if (db.exec(QStringLiteral("DELETE FROM clients WHERE id = ?"), a, &err) < 0)
            Msg::warn(this, QStringLiteral("Ошибка"), err);
        QTimer::singleShot(0, this, [this] { refresh(); });
    });
}

void ClientsPage::refresh() {
    const auto rows = Database::instance().select(
        QStringLiteral("SELECT c.id, c.full_name, c.phone, c.note, "
                       "(SELECT COUNT(*) FROM orders o WHERE o.client_id = c.id) "
                       "FROM clients c ORDER BY c.id"));
    m_table->setRowCount(0);
    int row = 0;
    for (const QVariantList& r : rows) {
        if (!Fmt::matches({r[1].toString(), r[2].toString()}, m_search->text()))
            continue;
        m_table->insertRow(row);
        Style::setText(m_table, row, 0, r[1].toString(), r[0].toInt());
        Style::setText(m_table, row, 1, r[2].toString());
        Style::setText(m_table, row, 2, r[4].toString());
        Style::setText(m_table, row, 3, r[3].toString());
        ++row;
    }
    Style::fitTable(m_table, 480);
}

// ===================== форма клиента =====================
ClientFormPage::ClientFormPage(QWidget* parent) : Page(parent) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(36, 32, 36, 36);
    v->setSpacing(8);
    v->addWidget(Style::label(QStringLiteral("Введите контактные данные клиента"), "hint"));
    v->addSpacing(10);

    m_name = new QLineEdit;
    m_name->setPlaceholderText(QStringLiteral("Иванов Иван Иванович"));
    m_name->setMaxLength(120);
    m_phone = new QLineEdit;
    m_phone->setInputMask(QString::fromLatin1(Fmt::phoneMask()));
    m_note = new QLineEdit;
    m_note->setPlaceholderText(QStringLiteral("Дополнительная информация (необязательно)"));
    m_note->setMaxLength(250);
    m_name->setMaximumWidth(500);
    m_phone->setMaximumWidth(500);
    m_note->setMaximumWidth(700);

    v->addWidget(Style::label(QStringLiteral("ФИО клиента"), "fieldLabel"));
    v->addWidget(m_name);
    v->addSpacing(12);
    v->addWidget(Style::label(QStringLiteral("Телефон"), "fieldLabel"));
    v->addWidget(m_phone);
    v->addSpacing(12);
    v->addWidget(Style::label(QStringLiteral("Примечание"), "fieldLabel"));
    v->addWidget(m_note);
    v->addSpacing(22);

    auto* btns = new QHBoxLayout;
    auto* save = Style::button(QStringLiteral("Сохранить клиента"), "primary");
    save->setMinimumWidth(200);
    auto* cancel = Style::button(QStringLiteral("Отмена"), "secondary");
    cancel->setMinimumWidth(130);
    btns->addWidget(save);
    btns->addWidget(cancel);
    btns->addStretch(1);
    v->addLayout(btns);
    v->addStretch(1);

    connect(save, &QPushButton::clicked, this, &ClientFormPage::save);
    connect(cancel, &QPushButton::clicked, this, [this] { emit navigate(QStringLiteral("clients")); });
}

void ClientFormPage::prepare(int id) {
    m_id = id;
    m_name->clear();
    m_phone->clear();
    m_note->clear();
    if (id > 0) {
        const auto rows = Database::instance().select(
            QStringLiteral("SELECT full_name, phone, note FROM clients WHERE id = ?"), {id});
        if (!rows.isEmpty()) {
            m_name->setText(rows[0][0].toString());
            m_phone->setText(rows[0][1].toString());
            m_note->setText(rows[0][2].toString());
        }
    }
    m_name->setFocus();
}

void ClientFormPage::save() {
    const QString name = m_name->text().simplified();
    if (name.size() < 3) {
        Msg::warn(this, QStringLiteral("Проверьте данные"), QStringLiteral("Введите ФИО клиента."));
        return;
    }
    if (!m_phone->hasAcceptableInput()) {
        Msg::warn(this, QStringLiteral("Проверьте данные"), QStringLiteral("Введите номер телефона полностью."));
        return;
    }
    QString err;
    int n;
    if (m_id > 0)
        n = Database::instance().exec(QStringLiteral("UPDATE clients SET full_name = ?, phone = ?, note = ? WHERE id = ?"),
                                      {name, m_phone->text(), m_note->text().trimmed(), m_id}, &err);
    else
        n = Database::instance().exec(QStringLiteral("INSERT INTO clients(full_name, phone, note) VALUES (?,?,?)"),
                                      {name, m_phone->text(), m_note->text().trimmed()}, &err);
    if (n < 0) {
        Msg::warn(this, QStringLiteral("Ошибка"), QStringLiteral("Не удалось сохранить клиента:\n") + err);
        return;
    }
    emit navigate(QStringLiteral("clients"));
}
