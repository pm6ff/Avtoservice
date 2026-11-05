#pragma once
#include "Page.h"

class QComboBox;
class QFrame;
class QLabel;
class QLineEdit;
class Pager;
class QPushButton;
class QTableWidget;

// Список заказ-нарядов. У приёмщика — статусы выбираются прямо в таблице,
// у администратора — кнопка «Открыть». Двойной щелчок по строке открывает подробности.
class OrdersPage : public Page {
    Q_OBJECT
public:
    explicit OrdersPage(QWidget* parent = nullptr);
    void refresh() override;
private:
    void fillTable();
    void openOrder(int id);
    void closePanel();
    void deleteOrder(int id);
    void setStatus(int id, const QString& status);
    void addWork();
    void addPart();
    void removeWork();
    void removePart();
    QWidget* section(const QString& title, QTableWidget* table, QPushButton** add, QPushButton** del);

    bool m_admin;
    int m_openId = 0;
    QLineEdit* m_search;
    QTableWidget* m_table;
    Pager* m_pager;
    // панель подробностей
    QFrame* m_panel;
    QLabel* m_panelTitle;
    QLabel* m_total;
    QTableWidget* m_info;
    QTableWidget* m_works;
    QTableWidget* m_parts;
    QComboBox* m_status;
};
