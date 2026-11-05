#include "CarsPage.h"
#include "Common.h"
#include "Database.h"
#include "Style.h"
#include <QComboBox>
#include <QDate>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

CarsPage::CarsPage(QWidget* parent) : Page(parent) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(36, 28, 36, 36);
    v->setSpacing(12);

    auto* top = new QHBoxLayout;
    auto* sv = new QVBoxLayout;
    sv->setSpacing(6);
    sv->addWidget(Style::label(QStringLiteral("Поиск автомобиля"), "fieldLabel"));
    m_search = new QLineEdit;
    m_search->setPlaceholderText(QStringLiteral("Марка, модель или госномер"));
    m_search->setFixedWidth(500);
    sv->addWidget(m_search);
    top->addLayout(sv);
    top->addStretch(1);
    auto* del = Style::button(QStringLiteral("Удалить"), "danger");
    auto* add = Style::button(QStringLiteral("+ Добавить автомобиль"), "primary");
    add->setMinimumWidth(220);
    top->addWidget(del, 0, Qt::AlignBottom);
    top->addWidget(add, 0, Qt::AlignBottom);
    v->addLayout(top);
    v->addSpacing(6);

    m_table = Style::table({QStringLiteral("Марка и модель"), QStringLiteral("Госномер"), QStringLiteral("Год"),
                            QStringLiteral("Владелец"), QStringLiteral("VIN"), QStringLiteral("Номер Телефона")});
    v->addWidget(m_table);
    v->addStretch(1);

    connect(m_search, &QLineEdit::textChanged, this, [this] { refresh(); });
    connect(add, &QPushButton::clicked, this, [this] { emit navigate(QStringLiteral("car_form"), 0); });
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (auto* it = m_table->item(row, 0)) emit navigate(QStringLiteral("car_form"), it->data(Qt::UserRole).toInt());
    });
    connect(del, &QPushButton::clicked, this, [this] {
        const int id = Style::currentId(m_table);
        if (id < 0) {
            Msg::info(this, QStringLiteral("Удаление"), QStringLiteral("Выберите автомобиль в таблице."));
            return;
        }
        if (!Msg::confirm(this, QStringLiteral("Удаление автомобиля"), QStringLiteral("Удалить выбранный автомобиль?")))
            return;
        QString err;
        if (Database::instance().exec(QStringLiteral("DELETE FROM cars WHERE id = ?"), {id}, &err) < 0) {
            if (Database::isForeignKeyError(err))
                Msg::warn(this, QStringLiteral("Удаление невозможно"),
                          QStringLiteral("Автомобиль указан в заказ-нарядах. Сначала удалите их."));
            else
                Msg::warn(this, QStringLiteral("Ошибка"), err);
        }
        QTimer::singleShot(0, this, [this] { refresh(); });
    });
}

void CarsPage::refresh() {
    const auto rows = Database::instance().select(
        QStringLiteral("SELECT car.id, car.model, car.plate, car.year, c.full_name, car.vin, c.phone "
                       "FROM cars car JOIN clients c ON c.id = car.client_id ORDER BY car.id"));
    m_table->setRowCount(0);
    int row = 0;
    for (const QVariantList& r : rows) {
        if (!Fmt::matches({r[1].toString(), r[2].toString()}, m_search->text()))
            continue;
        m_table->insertRow(row);
        Style::setText(m_table, row, 0, r[1].toString(), r[0].toInt());
        Style::setText(m_table, row, 1, r[2].toString());
        Style::setText(m_table, row, 2, r[3].toString());
        Style::setText(m_table, row, 3, Fmt::shortName(r[4].toString()));
        Style::setText(m_table, row, 4, r[5].toString());
        Style::setText(m_table, row, 5, r[6].toString());
        ++row;
    }
    Style::fitTable(m_table, 480);
}

// ===================== форма автомобиля =====================
CarFormPage::CarFormPage(QWidget* parent) : Page(parent) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(36, 32, 36, 36);
    v->setSpacing(10);
    v->addWidget(Style::label(QStringLiteral("Введите данные автомобиля и владельца"), "hint"));
    v->addSpacing(8);

    m_owner = new QComboBox;
    m_owner->setEditable(true);
    m_owner->setInsertPolicy(QComboBox::NoInsert);
    m_owner->lineEdit()->setPlaceholderText(QStringLiteral("Введите ФИО клиента"));
    m_owner->lineEdit()->setMaxLength(120);
    m_model = new QLineEdit;
    m_model->setPlaceholderText(QStringLiteral("Например, Ford Focus"));
    m_model->setMaxLength(60);
    m_plate = new QLineEdit;
    m_plate->setPlaceholderText(QStringLiteral("A123BC 77"));
    m_plate->setMaxLength(12);
    m_year = new QLineEdit;
    m_year->setPlaceholderText(QStringLiteral("Например, 2018"));
    m_year->setValidator(new QIntValidator(1950, QDate::currentDate().year() + 1, this));
    m_vin = new QLineEdit;
    m_vin->setPlaceholderText(QStringLiteral("Идентификационный номер"));
    m_vin->setValidator(new QRegularExpressionValidator(QRegularExpression(QStringLiteral("[A-Za-z0-9]{0,17}")), this));
    m_phone = new QLineEdit;
    m_phone->setInputMask(QString::fromLatin1(Fmt::phoneMask()));

    auto* g = new QGridLayout;
    g->setHorizontalSpacing(40);
    g->setVerticalSpacing(8);
    g->setColumnMinimumWidth(0, 400);
    g->setColumnMinimumWidth(1, 400);
    g->addWidget(Style::label(QStringLiteral("Владелец"), "fieldLabel"), 0, 0);
    g->addWidget(Style::label(QStringLiteral("Марка и модель"), "fieldLabel"), 0, 1);
    g->addWidget(m_owner, 1, 0);
    g->addWidget(m_model, 1, 1);
    g->setRowMinimumHeight(2, 12);
    g->addWidget(Style::label(QStringLiteral("Государственный номер"), "fieldLabel"), 3, 0);
    g->addWidget(Style::label(QStringLiteral("Год выпуска"), "fieldLabel"), 3, 1);
    g->addWidget(m_plate, 4, 0);
    g->addWidget(m_year, 4, 1);
    g->setRowMinimumHeight(5, 12);
    g->addWidget(Style::label(QStringLiteral("VIN"), "fieldLabel"), 6, 0);
    g->addWidget(Style::label(QStringLiteral("Номер телефона"), "fieldLabel"), 6, 1);
    g->addWidget(m_vin, 7, 0);
    g->addWidget(m_phone, 7, 1);
    g->setColumnStretch(2, 1);
    v->addLayout(g);
    v->addSpacing(22);

    auto* btns = new QHBoxLayout;
    auto* save = Style::button(QStringLiteral("Сохранить автомобиль"), "primary");
    save->setMinimumWidth(220);
    auto* cancel = Style::button(QStringLiteral("Отмена"), "secondary");
    cancel->setMinimumWidth(130);
    btns->addWidget(save);
    btns->addWidget(cancel);
    btns->addStretch(1);
    v->addLayout(btns);
    v->addStretch(1);

    connect(m_owner, &QComboBox::editTextChanged, this, [this] { onOwnerChanged(); });
    connect(save, &QPushButton::clicked, this, &CarFormPage::save);
    connect(cancel, &QPushButton::clicked, this, [this] { emit navigate(QStringLiteral("cars")); });
}

void CarFormPage::prepare(int id) {
    m_id = id;
    auto& db = Database::instance();
    m_owner->blockSignals(true);
    m_owner->clear();
    for (const QVariantList& r : db.select(QStringLiteral("SELECT id, full_name FROM clients ORDER BY full_name")))
        m_owner->addItem(r[1].toString(), r[0].toInt());
    m_owner->setCurrentIndex(-1);
    m_owner->setEditText(QString());
    m_owner->blockSignals(false);
    m_model->clear();
    m_plate->clear();
    m_year->clear();
    m_vin->clear();
    m_phone->clear();
    m_phone->setEnabled(true);

    if (id > 0) {
        const auto rows = db.select(
            QStringLiteral("SELECT car.model, car.plate, car.year, car.vin, c.full_name FROM cars car "
                           "JOIN clients c ON c.id = car.client_id WHERE car.id = ?"), {id});
        if (!rows.isEmpty()) {
            m_model->setText(rows[0][0].toString());
            m_plate->setText(rows[0][1].toString());
            m_year->setText(rows[0][2].toString());
            m_vin->setText(rows[0][3].toString());
            m_owner->setEditText(rows[0][4].toString());
        }
    }
    onOwnerChanged();
}

// Если введённое ФИО совпало с существующим клиентом — телефон подставляется и блокируется,
// иначе будет создан новый клиент с указанным телефоном.
void CarFormPage::onOwnerChanged() {
    const QString name = m_owner->currentText().simplified();
    const auto rows = Database::instance().select(
        QStringLiteral("SELECT phone FROM clients WHERE full_name = ? LIMIT 1"), {name});
    if (!rows.isEmpty()) {
        m_phone->setText(rows[0][0].toString());
        m_phone->setEnabled(false);
    } else {
        if (!m_phone->isEnabled()) m_phone->clear();
        m_phone->setEnabled(true);
    }
}

void CarFormPage::save() {
    auto warn = [this](const QString& t) { Msg::warn(this, QStringLiteral("Проверьте данные"), t); };
    const QString owner = m_owner->currentText().simplified();
    const QString model = m_model->text().simplified();
    const QString plate = m_plate->text().simplified().toUpper();
    const QString vin = m_vin->text().trimmed().toUpper();
    bool ok = false;
    const int year = m_year->text().toInt(&ok);

    if (owner.size() < 3) return warn(QStringLiteral("Укажите владельца автомобиля."));
    if (model.isEmpty()) return warn(QStringLiteral("Укажите марку и модель."));
    if (plate.isEmpty()) return warn(QStringLiteral("Укажите государственный номер."));
    if (!ok || year < 1950 || year > QDate::currentDate().year() + 1) return warn(QStringLiteral("Укажите корректный год выпуска."));
    if (vin.size() < 5) return warn(QStringLiteral("VIN должен содержать не менее 5 символов (буквы и цифры)."));

    auto& db = Database::instance();
    db.transaction();
    QString err;
    int clientId = db.scalar(QStringLiteral("SELECT id FROM clients WHERE full_name = ? LIMIT 1"), {owner}).toInt();
    if (clientId == 0) {
        if (!m_phone->hasAcceptableInput()) {
            db.rollback();
            return warn(QStringLiteral("Клиента с таким ФИО нет в базе — введите его номер телефона полностью."));
        }
        qint64 newId = 0;
        if (db.exec(QStringLiteral("INSERT INTO clients(full_name, phone, note) VALUES (?,?,'')"),
                    {owner, m_phone->text()}, &err, &newId) < 0) {
            db.rollback();
            Msg::warn(this, QStringLiteral("Ошибка"), err);
            return;
        }
        clientId = int(newId);
    }
    int n;
    if (m_id > 0)
        n = db.exec(QStringLiteral("UPDATE cars SET client_id = ?, model = ?, plate = ?, year = ?, vin = ? WHERE id = ?"),
                    {clientId, model, plate, year, vin, m_id}, &err);
    else
        n = db.exec(QStringLiteral("INSERT INTO cars(client_id, model, plate, year, vin) VALUES (?,?,?,?,?)"),
                    {clientId, model, plate, year, vin}, &err);
    if (n < 0) {
        db.rollback();
        if (Database::isUniqueError(err))
            warn(QStringLiteral("Автомобиль с таким госномером или VIN уже есть в базе."));
        else
            Msg::warn(this, QStringLiteral("Ошибка"), QStringLiteral("Не удалось сохранить автомобиль:\n") + err);
        return;
    }
    db.commit();
    emit navigate(QStringLiteral("cars"));
}

