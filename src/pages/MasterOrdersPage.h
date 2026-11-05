#pragma once
#include "Page.h"

class QLineEdit;
class Pager;
class QTableWidget;

// Окна мастера: «Главная» (задачи на сегодня), «Мои заказ-наряды», «История работ».
class MasterOrdersPage : public Page {
    Q_OBJECT
public:
    enum Mode { Home, Mine, History };
    explicit MasterOrdersPage(Mode mode, QWidget* parent = nullptr);
    void refresh() override;
private:
    void changeStatus(int orderId, const QString& status);
    Mode m_mode;
    QLineEdit* m_search;
    QTableWidget* m_table;
    Pager* m_pager;
};
