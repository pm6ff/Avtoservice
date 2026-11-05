#include "CatalogPage.h"
#include "Common.h"
#include "Database.h"
#include "Style.h"
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

CatalogPage::CatalogPage(QWidget* parent) : Page(parent) {
    m_admin = Session::instance().isAdmin();
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(36, 32, 36, 36);
    v->setSpacing(12);

    auto* top = new QHBoxLayout;
    auto* tv = new QVBoxLayout;
    tv->setSpacing(4);
    tv->addWidget(Style::label(QStringLiteral("Услуги и запчасти"), "h2"));
    tv->addWidget(Style::label(QStringLiteral("Справочник работ и используемых деталей"), "hint"));
    top->addLayout(tv);
    top->addStretch(1);
    v->addLayout(top);
    v->addSpacing(8);

    m_table = Style::table({QStringLiteral("Наименование"), QStringLiteral("Тип"),
                            QStringLiteral("Цена"), QStringLiteral("Единица")});
    v->addWidget(m_table);
    v->addStretch(1);

    if (m_admin) {
        auto* del = Style::button(QStringLiteral("Удалить"), "danger");
        auto* add = Style::button(QStringLiteral("+ Добавить позицию"), "primary");
        add->setMinimumWidth(200);
        top->addWidget(del, 0, Qt::AlignTop);
        top->addWidget(add, 0, Qt::AlignTop);
        connect(add, &QPushButton::clicked, this, [this] { emit navigate(QStringLiteral("catalog_form"), 0); });
        connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
            if (auto* it = m_table->item(row, 0)) emit navigate(QStringLiteral("catalog_form"), it->data(Qt::UserRole).toInt());
        });
        connect(del, &QPushButton::clicked, this, [this] {
            const int id = Style::currentId(m_table);
            if (id < 0) {
                Msg::info(this, QStringLiteral("Удаление"), QStringLiteral("Выберите позицию в таблице."));
                return;
            }
            if (!Msg::confirm(this, QStringLiteral("Удаление позиции"), QStringLiteral("Удалить выбранную позицию из справочника?")))
                return;
            // работы и запчасти в уже оформленных нарядах хранят собственную копию названия и цены
            Database::instance().exec(QStringLiteral("DELETE FROM catalog WHERE id = ?"), {id});
            QTimer::singleShot(0, this, [this] { refresh(); });
        });
    }
}

void CatalogPage::refresh() {
    const auto rows = Database::instance().select(
        QStringLiteral("SELECT id, name, kind, price, unit FROM catalog ORDER BY id"));
    m_table->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        Style::setText(m_table, i, 0, rows[i][1].toString(), rows[i][0].toInt());
        Style::setText(m_table, i, 1, rows[i][2].toString());
        Style::setText(m_table, i, 2, Fmt::money(rows[i][3].toDouble()));
        Style::setText(m_table, i, 3, rows[i][4].toString());
    }
    Style::fitTable(m_table, 500);
}

// ===================== форма позиции =====================
CatalogFormPage::CatalogFormPage(QWidget* parent) : Page(parent) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(36, 32, 36, 36);
    v->setSpacing(10);
    v->addWidget(Style::label(QStringLiteral("Добавление услуги или запчасти"), "h2"));
    v->addSpacing(22);

    m_kind = new QComboBox;
    m_kind->addItems(Kind::all());
    m_price = new QDoubleSpinBox;
    m_price->setRange(0, 9999999);
    m_price->setDecimals(2);
    m_price->setSuffix(QStringLiteral(" ₽"));
    m_price->setGroupSeparatorShown(true);
    m_name = new QLineEdit;
    m_name->setPlaceholderText(QStringLiteral("Например, Замена масла..."));
    m_name->setMaxLength(100);
    m_unit = new QLineEdit;
    m_unit->setPlaceholderText(QStringLiteral("Например, комплект"));
    m_unit->setMaxLength(20);

    auto* g = new QGridLayout;
    g->setHorizontalSpacing(40);
    g->setVerticalSpacing(8);
    g->setColumnMinimumWidth(0, 400);
    g->setColumnMinimumWidth(1, 400);
    g->addWidget(Style::label(QStringLiteral("Вид"), "fieldLabel"), 0, 0);
    g->addWidget(Style::label(QStringLiteral("Цена"), "fieldLabel"), 0, 1);
    g->addWidget(m_kind, 1, 0);
    g->addWidget(m_price, 1, 1);
    g->setRowMinimumHeight(2, 12);
    g->addWidget(Style::label(QStringLiteral("Наименование"), "fieldLabel"), 3, 0);
    g->addWidget(Style::label(QStringLiteral("Единица"), "fieldLabel"), 3, 1);
    g->addWidget(m_name, 4, 0);
    g->addWidget(m_unit, 4, 1);
    g->setColumnStretch(2, 1);
    v->addLayout(g);
    v->addSpacing(22);

    auto* btns = new QHBoxLayout;
    auto* save = Style::button(QStringLiteral("+ Добавить позицию"), "primary");
    save->setMinimumWidth(200);
    auto* cancel = Style::button(QStringLiteral("Отмена"), "secondary");
    cancel->setMinimumWidth(130);
    btns->addWidget(save);
    btns->addWidget(cancel);
    btns->addStretch(1);
    v->addLayout(btns);
    v->addStretch(1);

    connect(save, &QPushButton::clicked, this, &CatalogFormPage::save);
    connect(cancel, &QPushButton::clicked, this, [this] { emit navigate(QStringLiteral("catalog")); });
}

void CatalogFormPage::prepare(int id) {
    m_id = id;
    m_kind->setCurrentIndex(0);
    m_price->setValue(0);
    m_name->clear();
    m_unit->clear();
    if (id > 0) {
        const auto rows = Database::instance().select(
            QStringLiteral("SELECT name, kind, price, unit FROM catalog WHERE id = ?"), {id});
        if (!rows.isEmpty()) {
            m_name->setText(rows[0][0].toString());
            m_kind->setCurrentText(rows[0][1].toString());
            m_price->setValue(rows[0][2].toDouble());
            m_unit->setText(rows[0][3].toString());
        }
    }
}

void CatalogFormPage::save() {
    // защита на уровне окна: менять справочник может только администратор
    if (!Session::instance().isAdmin()) return;
    const QString name = m_name->text().simplified();
    const QString unit = m_unit->text().simplified();
    if (name.isEmpty()) {
        Msg::warn(this, QStringLiteral("Проверьте данные"), QStringLiteral("Введите наименование."));
        return;
    }
    if (unit.isEmpty()) {
        Msg::warn(this, QStringLiteral("Проверьте данные"), QStringLiteral("Укажите единицу измерения."));
        return;
    }
    QString err;
    int n;
    if (m_id > 0)
        n = Database::instance().exec(QStringLiteral("UPDATE catalog SET name = ?, kind = ?, price = ?, unit = ? WHERE id = ?"),
                                      {name, m_kind->currentText(), m_price->value(), unit, m_id}, &err);
    else
        n = Database::instance().exec(QStringLiteral("INSERT INTO catalog(name, kind, price, unit) VALUES (?,?,?,?)"),
                                      {name, m_kind->currentText(), m_price->value(), unit}, &err);
    if (n < 0) {
        Msg::warn(this, QStringLiteral("Ошибка"), QStringLiteral("Не удалось сохранить позицию:\n") + err);
        return;
    }
    emit navigate(QStringLiteral("catalog"));
}
