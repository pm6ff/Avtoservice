#pragma once
#include <QWidget>

class QHBoxLayout;

// Постраничная навигация: кнопки «‹ 1 2 3 … N ›» под таблицей.
// Использование: setTotal(число записей) -> показывать записи [first(), last()).
class Pager : public QWidget {
    Q_OBJECT
public:
    explicit Pager(int pageSize = 8, QWidget* parent = nullptr);

    int page() const { return m_page; }       // текущая страница (с 1)
    int pageCount() const { return m_pages; }
    int first() const;                        // индекс первой записи страницы (с 0)
    int last() const;                         // индекс за последней записью страницы
    void reset() { m_page = 1; }              // вернуться на первую страницу (например, при новом поиске)
    void setTotal(int total);                 // пересчитать страницы и перерисовать кнопки

signals:
    void pageChanged(int page);

private:
    void rebuild();
    void goTo(int page);

    int m_pageSize;
    int m_total = 0;
    int m_pages = 1;
    int m_page = 1;
    QHBoxLayout* m_layout;
};
