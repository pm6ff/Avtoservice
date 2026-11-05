#pragma once
#include <QWidget>

class QLineEdit;
class QLabel;
class QPushButton;

class LoginWindow : public QWidget {
    Q_OBJECT
public:
    explicit LoginWindow(QWidget* parent = nullptr);
    void reset();
signals:
    void loggedIn();
private:
    void tryLogin();
    QLineEdit* m_login;
    QLineEdit* m_password;
    QLabel* m_error;
    QPushButton* m_button;
    int m_failures = 0;
};
