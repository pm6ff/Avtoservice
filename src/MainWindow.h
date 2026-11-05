#pragma once
#include <QHash>
#include <QMainWindow>

class Page;
class QButtonGroup;
class QLabel;
class QPushButton;
class QStackedWidget;
class QVBoxLayout;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
signals:
    void logoutRequested();
private:
    void addNav(const QString& key, const QString& title, const QString& icon);
    void addPage(const QString& key, const QString& title, const QString& section, Page* page);
    void go(const QString& key, int id = 0);

    QStackedWidget* m_stack = nullptr;
    QLabel* m_title = nullptr;
    QVBoxLayout* m_navLayout = nullptr;
    QButtonGroup* m_navGroup = nullptr;
    QHash<QString, Page*> m_pages;
    QHash<QString, QString> m_titles;
    QHash<QString, QString> m_sections;
    QHash<QString, QPushButton*> m_navButtons;
};
