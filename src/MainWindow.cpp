#include "MainWindow.h"
#include "Common.h"
#include "Icons.h"
#include "Page.h"
#include "Style.h"
#include "pages/CarsPage.h"
#include "pages/CatalogPage.h"
#include "pages/ClientsPage.h"
#include "pages/DashboardPage.h"
#include "pages/EmployeesPage.h"
#include "pages/MasterOrdersPage.h"
#include "pages/OrderFormPage.h"
#include "pages/OrdersPage.h"
#include "pages/ProfilePage.h"
#include "pages/ReportsPage.h"
#include "pages/RolesPage.h"
#include <QButtonGroup>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    const User& u = Session::instance().user;
    setWindowTitle(QStringLiteral("Автосервис — учёт ремонтных работ и заказ-нарядов"));
    resize(1280, 820);
    setMinimumSize(1050, 640);

    auto* root = new QWidget;
    auto* h = new QHBoxLayout(root);
    h->setContentsMargins(0, 0, 0, 0);
    h->setSpacing(0);

    // ---------- боковая панель ----------
    auto* side = new QFrame;
    side->setObjectName(QStringLiteral("sidebar"));
    side->setFixedWidth(232);
    m_navLayout = new QVBoxLayout(side);
    m_navLayout->setContentsMargins(10, 18, 10, 14);
    m_navLayout->setSpacing(8);
    auto* brand = Style::label(QStringLiteral("АВТОСЕРВИС"), "brand");
    brand->setContentsMargins(10, 0, 0, 0);
    auto* sub = Style::label(QStringLiteral("Учёт ремонтных работ"), "muted");
    sub->setContentsMargins(10, 0, 0, 0);
    m_navLayout->addWidget(brand);
    m_navLayout->addWidget(sub);
    m_navLayout->addSpacing(18);
    m_navGroup = new QButtonGroup(this);
    m_navGroup->setExclusive(true);

    // ---------- правая часть ----------
    auto* right = new QWidget;
    auto* rv = new QVBoxLayout(right);
    rv->setContentsMargins(0, 0, 0, 0);
    rv->setSpacing(0);

    auto* top = new QFrame;
    top->setObjectName(QStringLiteral("topbar"));
    top->setFixedHeight(68);
    auto* th = new QHBoxLayout(top);
    th->setContentsMargins(32, 0, 14, 0);
    m_title = Style::label(QString(), "pageTitle");
    th->addWidget(m_title);
    th->addStretch(1);
    const QString roleText = u.role == Role::Reception ? QStringLiteral("Приёмщик СТО") : u.role;
    th->addWidget(Style::label(roleText, "roleLabel"));
    th->addSpacing(12);
    auto* logout = new QPushButton(QStringLiteral("Выйти"));
    logout->setProperty("logout", true);
    logout->setIcon(Icons::plain(QStringLiteral("logout"), QColor(QStringLiteral("#6b7385"))));
    logout->setIconSize(QSize(22, 22));
    logout->setCursor(Qt::PointingHandCursor);
    connect(logout, &QPushButton::clicked, this, &MainWindow::logoutRequested);
    th->addWidget(logout);
    rv->addWidget(top);

    m_stack = new QStackedWidget;
    m_stack->setObjectName(QStringLiteral("stack"));
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(m_stack);
    scroll->viewport()->setObjectName(QStringLiteral("viewport"));
    rv->addWidget(scroll, 1);

    h->addWidget(side);
    h->addWidget(right, 1);
    setCentralWidget(root);

    // ---------- состав окон зависит от роли  ----------
    if (u.role == Role::Master) {
        addNav("home", "Главная", "home");
        addNav("my_orders", "Мои заказ-наряды", "orders");
        addNav("history", "История работ", "car");
        addNav("profile", "Профиль", "wrench");
        addPage("home", "Главная", "home", new MasterOrdersPage(MasterOrdersPage::Home));
        addPage("my_orders", "Мои заказ-наряды", "my_orders", new MasterOrdersPage(MasterOrdersPage::Mine));
        addPage("history", "История работ", "history", new MasterOrdersPage(MasterOrdersPage::History));
        addPage("profile", "Профиль", "profile", new ProfilePage);
    } else {
        const bool admin = u.role == Role::Admin;
        addNav("home", "Главная", "home");
        addNav("orders", "Заказ-наряды", "orders");
        addNav("clients", "Клиенты", "client");
        addNav("cars", "Автомобили", "car");
        addNav("catalog", "Услуги и запчасти", "wrench");
        if (admin) {
            addNav("staff", "Сотрудники", "staff");
            addNav("reports", "Отчёты", "report");
            addNav("roles", "Роли", "shield");
        }
        addPage("home", "Главная", "home", new DashboardPage);
        addPage("orders", "Заказ-наряды", "orders", new OrdersPage);
        addPage("order_form", "Форма — заказ-наряд", "orders", new OrderFormPage);
        addPage("clients", "Клиенты", "clients", new ClientsPage);
        addPage("client_form", "Форма — клиента", "clients", new ClientFormPage);
        addPage("cars", "Автомобили", "cars", new CarsPage);
        addPage("car_form", "Форма — автомобиля", "cars", new CarFormPage);
        addPage("catalog", "Услуги и запчасти", "catalog", new CatalogPage);
        addPage("catalog_form", "Форма — Услуги и запчасти", "catalog", new CatalogFormPage);
        if (admin) {
            addPage("staff", "Сотрудники", "staff", new EmployeesPage);
            addPage("staff_form", "Добавление сотрудника", "staff", new EmployeeFormPage);
            addPage("reports", "Отчёты", "reports", new ReportsPage);
            addPage("roles", "Роли сотрудников", "roles", new RolesPage);
        }
    }

    m_navLayout->addSpacing(8);
    auto* line = new QFrame;
    line->setObjectName(QStringLiteral("divider"));
    line->setFixedHeight(1);
    m_navLayout->addWidget(line);
    m_navLayout->addStretch(1);
    auto* who = Style::label(QStringLiteral("Пользователь: ") + Fmt::shortName(u.fullName), "muted");
    who->setContentsMargins(10, 0, 0, 0);
    m_navLayout->addWidget(who);

    go(QStringLiteral("home"));
}

void MainWindow::addNav(const QString& key, const QString& title, const QString& icon) {
    auto* b = new QPushButton(title);
    b->setProperty("nav", true);
    b->setCheckable(true);
    b->setIcon(Icons::nav(icon));
    b->setIconSize(QSize(22, 22));
    b->setCursor(Qt::PointingHandCursor);
    m_navGroup->addButton(b);
    m_navLayout->addWidget(b);
    m_navButtons.insert(key, b);
    connect(b, &QPushButton::clicked, this, [this, key] { go(key); });
}

void MainWindow::addPage(const QString& key, const QString& title, const QString& section, Page* page) {
    m_stack->addWidget(page);
    m_pages.insert(key, page);
    m_titles.insert(key, title);
    m_sections.insert(key, section);
    connect(page, &Page::navigate, this, [this](const QString& k, int id) { go(k, id); });
}

void MainWindow::go(const QString& key, int id) {
    Page* page = m_pages.value(key, nullptr);
    if (!page)
        return;
    page->prepare(id);
    m_stack->setCurrentWidget(page);
    m_title->setText(m_titles.value(key));
    if (QPushButton* b = m_navButtons.value(m_sections.value(key), nullptr))
        b->setChecked(true);
}
