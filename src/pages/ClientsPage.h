#pragma once
#include "Page.h"

class QLineEdit;
class QTableWidget;

class ClientsPage : public Page {
    Q_OBJECT
public:
    explicit ClientsPage(QWidget* parent = nullptr);
    void refresh() override;
private:
    QLineEdit* m_search;
    QTableWidget* m_table;
};

class ClientFormPage : public Page {
    Q_OBJECT
public:
    explicit ClientFormPage(QWidget* parent = nullptr);
    void prepare(int id = 0) override;
private:
    void save();
    int m_id = 0;
    QLineEdit* m_name;
    QLineEdit* m_phone;
    QLineEdit* m_note;
};
