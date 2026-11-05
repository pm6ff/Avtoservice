#include "Pager.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QPushButton>

Pager::Pager(int pageSize, QWidget* parent) : QWidget(parent), m_pageSize(qMax(1, pageSize)) {
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 10, 0, 0);
    m_layout->setSpacing(6);
    hide();
}

int Pager::first() const { return (m_page - 1) * m_pageSize; }
int Pager::last() const { return qMin(m_total, m_page * m_pageSize); }

void Pager::setTotal(int total) {
    m_total = qMax(0, total);
    m_pages = qMax(1, (m_total + m_pageSize - 1) / m_pageSize);
    m_page = qBound(1, m_page, m_pages);   // если записей стало меньше — остаёмся на последней странице
    rebuild();
}

void Pager::goTo(int page) {
    page = qBound(1, page, m_pages);
    if (page == m_page)
        return;
    m_page = page;
    emit pageChanged(m_page);
}

void Pager::rebuild() {
    while (QLayoutItem* item = m_layout->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->hide();
            w->deleteLater();      // кнопка может быть источником текущего сигнала — удаляем отложенно
        }
        delete item;
    }
    if (m_pages <= 1) {            // одна страница — навигация не нужна
        hide();
        return;
    }
    show();

    auto addButton = [this](const QString& text, int target, bool current, bool enabled) {
        auto* b = new QPushButton(text);
        b->setProperty("pager", true);
        b->setProperty("current", current);
        b->setEnabled(enabled);
        b->setCursor(Qt::PointingHandCursor);
        connect(b, &QPushButton::clicked, this, [this, target] { goTo(target); });
        m_layout->addWidget(b);
    };

    addButton(QStringLiteral("‹"), m_page - 1, false, m_page > 1);

    // 1 … (p-1) p (p+1) … N
    QList<int> numbers;
    numbers << 1;
    for (int p = m_page - 1; p <= m_page + 1; ++p)
        if (p > 1 && p < m_pages)
            numbers << p;
    numbers << m_pages;

    int prev = 0;
    for (int p : numbers) {
        if (p - prev > 1)
            m_layout->addWidget(new QLabel(QStringLiteral("…")));
        addButton(QString::number(p), p, p == m_page, true);
        prev = p;
    }

    addButton(QStringLiteral("›"), m_page + 1, false, m_page < m_pages);
    m_layout->addStretch(1);

    auto* info = new QLabel(QStringLiteral("Записи %1–%2 из %3").arg(first() + 1).arg(last()).arg(m_total));
    info->setObjectName(QStringLiteral("muted"));
    m_layout->addWidget(info);
}
