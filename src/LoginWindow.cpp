#include "LoginWindow.h"
#include "Database.h"
#include "Style.h"
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

LoginWindow::LoginWindow(QWidget* parent) : QWidget(parent) {
    setObjectName(QStringLiteral("loginRoot"));
    setAttribute(Qt::WA_StyledBackground, true);
    setWindowTitle(QStringLiteral("Автосервис — вход в систему"));
    resize(760, 720);
    setMinimumSize(560, 640);

    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(24, 24, 24, 24);

    auto* card = new QFrame;
    card->setObjectName(QStringLiteral("loginCard"));
    card->setFixedWidth(500);
    outer->addWidget(card, 0, Qt::AlignCenter);

    auto* v = new QVBoxLayout(card);
    v->setContentsMargins(60, 44, 60, 36);
    v->setSpacing(8);

    auto* title = Style::label(QStringLiteral("АВТОСЕРВИС"), "loginTitle");
    title->setAlignment(Qt::AlignCenter);
    auto* sub = Style::label(QStringLiteral("Учёт ремонтных работ и заказ-нарядов"), "hint");
    sub->setAlignment(Qt::AlignCenter);
    v->addWidget(title);
    v->addWidget(sub);
    v->addSpacing(26);
    v->addWidget(Style::label(QStringLiteral("Вход в систему"), "h1"));
    v->addSpacing(10);

    v->addWidget(Style::label(QStringLiteral("Логин"), "fieldLabel"));
    m_login = new QLineEdit;
    m_login->setPlaceholderText(QStringLiteral("Введите логин"));
    m_login->setMaxLength(64);
    v->addWidget(m_login);
    v->addSpacing(8);

    v->addWidget(Style::label(QStringLiteral("Пароль"), "fieldLabel"));
    m_password = new QLineEdit;
    m_password->setPlaceholderText(QStringLiteral("Введите пароль"));
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setMaxLength(128);
    v->addWidget(m_password);

    m_error = Style::label(QString(), "error");
    m_error->setWordWrap(true);
    m_error->hide();
    v->addWidget(m_error);
    v->addSpacing(14);

    m_button = Style::button(QStringLiteral("Войти"), "primary");
    m_button->setMinimumHeight(48);
    m_button->setDefault(true);
    v->addWidget(m_button);
    v->addSpacing(14);

    auto* foot = Style::label(QStringLiteral("Доступ только для сотрудников автосервиса"), "muted");
    foot->setAlignment(Qt::AlignCenter);
    v->addWidget(foot);

    connect(m_button, &QPushButton::clicked, this, &LoginWindow::tryLogin);
    connect(m_login, &QLineEdit::returnPressed, this, [this] { m_password->setFocus(); });
    connect(m_password, &QLineEdit::returnPressed, this, &LoginWindow::tryLogin);
}

void LoginWindow::reset() {
    m_login->clear();
    m_password->clear();
    m_error->hide();
    m_failures = 0;
    m_button->setEnabled(true);
    m_login->setFocus();
}

void LoginWindow::tryLogin() {
    if (!m_button->isEnabled())
        return;
    if (m_login->text().trimmed().isEmpty() || m_password->text().isEmpty()) {
        m_error->setText(QStringLiteral("Введите логин и пароль."));
        m_error->show();
        return;
    }
    User u;
    if (Database::instance().authenticate(m_login->text(), m_password->text(), &u)) {
        Session::instance().user = u;
        m_password->clear();
        m_error->hide();
        m_failures = 0;
        emit loggedIn();
        return;
    }
    m_password->clear();
    if (++m_failures >= 5) {
        m_error->setText(QStringLiteral("Слишком много неудачных попыток. Подождите 10 секунд."));
        m_button->setEnabled(false);
        QTimer::singleShot(10000, this, [this] {
            m_failures = 0;
            m_button->setEnabled(true);
            m_error->hide();
        });
    } else {
        m_error->setText(QStringLiteral("Неверный логин или пароль."));
    }
    m_error->show();
    m_password->setFocus();
}
