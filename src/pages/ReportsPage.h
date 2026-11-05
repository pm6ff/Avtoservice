#pragma once
#include "Page.h"
#include <QList>

class QDateEdit;
class QLabel;
class QTableWidget;

class ReportsPage : public Page {
    Q_OBJECT
public:
    explicit ReportsPage(QWidget* parent = nullptr);
    void prepare(int id = 0) override;
    void refresh() override;
private:
    void generate();                 // «Сформировать отчёт»: обновить данные и сохранить PDF
    QDateEdit* m_from;
    QDateEdit* m_to;
    QList<QLabel*> m_values;
    QTableWidget* m_table;
};
