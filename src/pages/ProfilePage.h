#pragma once
#include "Page.h"

class QLabel;

class ProfilePage : public Page {
    Q_OBJECT
public:
    explicit ProfilePage(QWidget* parent = nullptr);
    void refresh() override;
private:
    void edit();
    QLabel* m_values[5];
};
