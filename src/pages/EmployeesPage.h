#pragma once
#include "Page.h"

class QComboBox;
class QLineEdit;
class QTableWidget;

class EmployeesPage : public Page {
    Q_OBJECT
public:
    explicit EmployeesPage(QWidget* parent = nullptr);
    void refresh() override;
private:
    QTableWidget* m_table;
};

class EmployeeFormPage : public Page {
    Q_OBJECT
public:
    explicit EmployeeFormPage(QWidget* parent = nullptr);
    void prepare(int id = 0) override;
private:
    void save();
    int m_id = 0;
    QLineEdit* m_name;
    QComboBox* m_role;
    QLineEdit* m_phone;
    QComboBox* m_status;
    QLineEdit* m_login;
    QLineEdit* m_password;
};
