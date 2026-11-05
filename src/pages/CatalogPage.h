#pragma once
#include "Page.h"

class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QTableWidget;

// Справочник. Приёмщик только просматривает, администратор — добавляет, правит (двойной щелчок) и удаляет.
class CatalogPage : public Page {
    Q_OBJECT
public:
    explicit CatalogPage(QWidget* parent = nullptr);
    void refresh() override;
private:
    bool m_admin;
    QTableWidget* m_table;
};

class CatalogFormPage : public Page {
    Q_OBJECT
public:
    explicit CatalogFormPage(QWidget* parent = nullptr);
    void prepare(int id = 0) override;
private:
    void save();
    int m_id = 0;
    QComboBox* m_kind;
    QDoubleSpinBox* m_price;
    QLineEdit* m_name;
    QLineEdit* m_unit;
};
