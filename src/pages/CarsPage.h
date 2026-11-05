#pragma once
#include "Page.h"

class QComboBox;
class QLineEdit;
class QTableWidget;

class CarsPage : public Page {
    Q_OBJECT
public:
    explicit CarsPage(QWidget* parent = nullptr);
    void refresh() override;
private:
    QLineEdit* m_search;
    QTableWidget* m_table;
};

class CarFormPage : public Page {
    Q_OBJECT
public:
    explicit CarFormPage(QWidget* parent = nullptr);
    void prepare(int id = 0) override;
private:
    void onOwnerChanged();
    void save();
    int m_id = 0;
    QComboBox* m_owner;     // редактируемый: можно выбрать клиента или ввести нового
    QLineEdit* m_model;
    QLineEdit* m_plate;
    QLineEdit* m_year;
    QLineEdit* m_vin;
    QLineEdit* m_phone;
};
