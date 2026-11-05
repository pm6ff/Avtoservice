# Автосервис — учёт ремонтных работ и заказ-нарядов

Десктопное приложение (C++17 + Qt Widgets + SQLite) по макетам из Figma (отчёт УП.01).
Три роли — **Приёмщик**, **Мастер**, **Администратор** — у каждой свой набор окон.

## Что нужно установить

| Что | Linux (Debian/Ubuntu) | Windows / macOS |
|---|---|---|
| Компилятор C++17, CMake ≥ 3.16 | `sudo apt install build-essential cmake ninja-build` | MSVC / MinGW / Xcode CLT + CMake |
| Qt 6 (Widgets, Sql, Network, Svg) + драйвер SQLite | `sudo apt install qt6-base-dev qt6-svg-dev libqt6sql6-sqlite` | Qt Online Installer (Qt 6.x, не забудьте компонент Svg) |

Qt 5.15 тоже подойдёт.

В VSCodium поставьте расширения (Open VSX): **clangd**, **CodeLLDB**, при желании **CMake** (twxs.cmake) — список уже в `.vscode/extensions.json`.
(Microsoft C/C++ в VSCodium недоступен, поэтому отладка настроена через CodeLLDB.)

## Сборка и запуск

В VSCodium: `Ctrl+Shift+B` (сборка) → задача **Run AutoService**.
Или в терминале:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
./build/AutoService
```

На Windows/macOS, если CMake не находит Qt, задайте путь: `cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.x/mingw_64`
(или переменную окружения `CMAKE_PREFIX_PATH`).

## Демо-учётки

| Роль | Логин | Пароль |
|---|---|---|
| Администратор | `admin` | `admin123` |
| Приёмщик | `reception` | `reception123` |
| Мастер | `master01` (также `master02`, `master03`) | `master123` |

База создаётся автоматически при первом запуске с демо-данными из макетов
(Linux: `~/.local/share/NEK/AutoService/autoservice.sqlite`). Другой путь — переменная `AUTOSERVICE_DB`.
Чтобы сбросить данные — удалите файл базы.

## Окна по ролям

**Приёмщик:** Главная · Заказ-наряды (+ форма) · Клиенты (+ форма) · Автомобили (+ форма) · Услуги и запчасти (просмотр)
**Мастер:** Главная (задачи) · Мои заказ-наряды · История работ · Профиль
**Администратор:** всё из списка приёмщика + Сотрудники (+ форма) · Отчёты · Роли; в «Услугах и запчастях» — добавление/удаление.

Действия: поиск в таблицах, добавление/удаление, смена статуса прямо в таблице, двойной щелчок по строке —
редактирование (клиент, автомобиль, позиция, сотрудник) или подробности заказ-наряда; в подробностях — добавление
работ и запчастей, итог считается автоматически.

## Архитектура

- `Database` — Singleton, SQLite, только параметризованные запросы, пароли PBKDF2-SHA256 + соль
- `Repository` — выборки заказ-нарядов с автоматическим расчётом суммы
- `MainWindow` + `Page` — боковое меню по роли, страницы-«окна»
- `pages/*` — экраны и формы (View + Controller), `Common`, `Style`, `Icons` — общее
