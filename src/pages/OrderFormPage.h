#pragma once
#include "Page.h"

class QComboBox;
class QDateEdit;
class QLineEdit;

class OrderFormPage : public Page {
    Q_OBJECT
public:
    explicit OrderFormPage(QWidget* parent = nullptr);
    void prepare(int id = 0) override;
private:
    void loadCars();
    void save();
    QComboBox* m_client;
    QComboBox* m_car;
    QDateEdit* m_date;
    QComboBox* m_master;
    QLineEdit* m_description;
    QComboBox* m_status;
};
