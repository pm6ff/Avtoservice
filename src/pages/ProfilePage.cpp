#include "ProfilePage.h"
#include "Common.h"
#include "Database.h"
#include "Style.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

ProfilePage::ProfilePage(QWidget* parent) : Page(parent) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(36, 32, 36, 36);
    v->setSpacing(14);

    auto* top = new QHBoxLayout;
    top->addWidget(Style::label(QStringLiteral("Личные данные"), "fieldLabel"));
    top->addStretch(1);
    auto* editBtn = Style::button(QStringLiteral("Изменить профиль"), "primary");
    editBtn->setMinimumWidth(230);
    top->addWidget(editBtn);
    v->addLayout(top);

    auto* card = Style::card();
    card->setMaximumWidth(720);
    auto* cv = new QVBoxLayout(card);
    cv->setContentsMargins(24, 22, 24, 26);
    cv->setSpacing(16);
    cv->addWidget(Style::label(QStringLiteral("Личные данные сотрудника"), "h2"));
    const QStringList keys = {QStringLiteral("ФИО"), QStringLiteral("Должность"), QStringLiteral("Телефон"),
                              QStringLiteral("Логин"), QStringLiteral("Статус")};
    auto* g = new QGridLayout;
    g->setVerticalSpacing(16);
    g->setColumnMinimumWidth(0, 200);
    for (int i = 0; i < keys.size(); ++i) {
        g->addWidget(Style::label(keys[i], "kvKey"), i, 0);
        m_values[i] = Style::label(QString(), "kvValue");
        g->addWidget(m_values[i], i, 1);
    }
    g->setColumnStretch(1, 1);
    cv->addLayout(g);
    auto* tr = new QHBoxLayout;
    tr->addWidget(card, 1);
    tr->addStretch(1);
    v->addLayout(tr);
    v->addStretch(1);

    connect(editBtn, &QPushButton::clicked, this, &ProfilePage::edit);
}

void ProfilePage::refresh() {
    const auto rows = Database::instance().select(
        QStringLiteral("SELECT full_name, role, phone, login, status FROM employees WHERE id = ?"),
        {Session::instance().user.id});
    if (rows.isEmpty()) return;
    for (int i = 0; i < 5; ++i) m_values[i]->setText(rows[0][i].toString());
}

void ProfilePage::edit() {
    User& me = Session::instance().user;
    QDialog dlg(this);
    dlg.setWindowTitle(QStringLiteral("Изменение профиля"));
    dlg.setMinimumWidth(460);
    auto* form = new QFormLayout(&dlg);
    auto* name = new QLineEdit(me.fullName);
    name->setMaxLength(120);
    auto* phone = new QLineEdit;
    phone->setInputMask(QString::fromLatin1(Fmt::phoneMask()));
    phone->setText(me.phone);
    auto* pw1 = new QLineEdit;
    pw1->setEchoMode(QLineEdit::Password);
    pw1->setPlaceholderText(QStringLiteral("Оставьте пустым, чтобы не менять"));
    auto* pw2 = new QLineEdit;
    pw2->setEchoMode(QLineEdit::Password);
    pw2->setPlaceholderText(QStringLiteral("Повторите новый пароль"));
    form->addRow(QStringLiteral("ФИО"), name);
    form->addRow(QStringLiteral("Телефон"), phone);
    form->addRow(QStringLiteral("Новый пароль"), pw1);
    form->addRow(QStringLiteral("Повтор пароля"), pw2);
    auto* bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    bb->button(QDialogButtonBox::Ok)->setText(QStringLiteral("Сохранить"));
    bb->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("Отмена"));
    form->addRow(bb);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(bb, &QDialogButtonBox::accepted, &dlg, [&] {
        if (name->text().simplified().size() < 3) {
            Msg::warn(&dlg, QStringLiteral("Проверьте данные"), QStringLiteral("Введите ФИО."));
        } else if (!phone->hasAcceptableInput()) {
            Msg::warn(&dlg, QStringLiteral("Проверьте данные"), QStringLiteral("Введите номер телефона полностью."));
        } else if (!pw1->text().isEmpty() && pw1->text().size() < 8) {
            Msg::warn(&dlg, QStringLiteral("Проверьте данные"), QStringLiteral("Пароль должен содержать не менее 8 символов."));
        } else if (pw1->text() != pw2->text()) {
            Msg::warn(&dlg, QStringLiteral("Проверьте данные"), QStringLiteral("Пароли не совпадают."));
        } else {
            dlg.accept();
        }
    });
    if (dlg.exec() != QDialog::Accepted) return;

    auto& db = Database::instance();
    // меняем только свою запись (id берётся из сессии, а не из интерфейса)
    if (db.exec(QStringLiteral("UPDATE employees SET full_name = ?, phone = ? WHERE id = ?"),
                {name->text().simplified(), phone->text(), me.id}) < 0) {
        Msg::warn(this, QStringLiteral("Ошибка"), QStringLiteral("Не удалось сохранить профиль."));
        return;
    }
    if (!pw1->text().isEmpty()) db.setPassword(me.id, pw1->text());
    me.fullName = name->text().simplified();
    me.phone = phone->text();
    refresh();
}
