#pragma once
#include "Page.h"

class QTableWidget;

class RolesPage : public Page {
    Q_OBJECT
public:
    explicit RolesPage(QWidget* parent = nullptr);
    void refresh() override;
private:
    void changeRole(int id, const QString& newRole);
    QTableWidget* m_table;
};
