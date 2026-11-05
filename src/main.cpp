#include "Database.h"
#include "LoginWindow.h"
#include "MainWindow.h"
#include "Style.h"
#include <QApplication>
#include <QIcon>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

int main(int argc, char* argv[]) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("AutoService"));
    QApplication::setOrganizationName(QStringLiteral("NEK"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icon.png")));
    app.setStyleSheet(Style::sheet());

    // Путь к БД можно переопределить переменной окружения AUTOSERVICE_DB
    QString dbPath = qEnvironmentVariable("AUTOSERVICE_DB");
    if (dbPath.isEmpty())
        dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                 QStringLiteral("/autoservice.sqlite");

    QString err;
    if (!Database::instance().open(dbPath, &err)) {
        QMessageBox::critical(nullptr, QStringLiteral("Ошибка базы данных"),
                              QStringLiteral("Не удалось открыть базу данных:\n%1\n\n"
                                             "Проверьте, что установлен драйвер Qt SQLite.").arg(err));
        return 1;
    }

    LoginWindow login;
    MainWindow* window = nullptr;

    QObject::connect(&login, &LoginWindow::loggedIn, [&]() {
        window = new MainWindow();
        QObject::connect(window, &MainWindow::logoutRequested, [&]() {
            Session::instance().clear();
            login.reset();
            login.show();
            window->deleteLater();
            window = nullptr;
        });
        window->show();
        login.hide();
    });

    login.show();
    login.reset();
    return app.exec();
}
