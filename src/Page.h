#pragma once
#include <QWidget>

// Базовый класс всех «окон» внутри главного окна.
class Page : public QWidget {
    Q_OBJECT
public:
    explicit Page(QWidget* parent = nullptr) : QWidget(parent) {}
    // Вызывается перед показом. Для форм id = 0 — создание, id > 0 — редактирование.
    virtual void prepare(int id = 0) { Q_UNUSED(id); refresh(); }
    virtual void refresh() {}
signals:
    // Просьба главному окну открыть другую страницу/форму.
    void navigate(const QString& key, int id = 0);
};
