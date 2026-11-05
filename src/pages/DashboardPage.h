#pragma once
#include "Page.h"
#include <QList>

class QLabel;
class QTableWidget;

// Главная для приёмщика (3 карточки) и администратора (4 карточки).
class DashboardPage : public Page {
    Q_OBJECT
public:
    explicit DashboardPage(QWidget* parent = nullptr);
    void refresh() override;
private:
    bool m_admin;
    QList<QLabel*> m_values;
    QTableWidget* m_table;
};
