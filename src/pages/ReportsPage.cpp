#include "ReportsPage.h"
#include "Common.h"
#include "Repository.h"
#include "Style.h"
#include <QDate>
#include <QDateEdit>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMarginsF>
#include <QMessageBox>
#include <QPageSize>
#include <QPdfWriter>
#include <QPushButton>
#include <QTableWidget>
#include <QTextDocument>
#include <QVBoxLayout>

ReportsPage::ReportsPage(QWidget* parent) : Page(parent) {
    auto* v = new QVBoxLayout(this);
    v->setContentsMargins(36, 28, 36, 36);
    v->setSpacing(16);

    // ----- выбор периода и кнопка -----
    auto* period = Style::card();
    period->setMaximumWidth(820);
    auto* pv = new QVBoxLayout(period);
    pv->setContentsMargins(18, 14, 18, 18);
    pv->addWidget(Style::label(QStringLiteral("Период отчёта"), "h2"));
    auto* row = new QHBoxLayout;
    row->setSpacing(24);
    auto field = [&](const QString& caption, QDateEdit*& edit) -> void {
        auto* col = new QVBoxLayout;
        col->setSpacing(4);
        col->addWidget(Style::label(caption, "hint"));
        edit = new QDateEdit;
        edit->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
        edit->setCalendarPopup(true);
        col->addWidget(edit);
        row->addLayout(col, 1);
    };
    field(QStringLiteral("С даты"), m_from);
    field(QStringLiteral("По дату"), m_to);
    auto* gen = Style::button(QStringLiteral("Сформировать отчёт"), "primary");
    gen->setMinimumWidth(190);
    row->addWidget(gen, 0, Qt::AlignBottom);
    pv->addLayout(row);
    v->addWidget(period);

    // ----- карточки с показателями -----
    const QStringList captions = {QStringLiteral("Всего заказов"), QStringLiteral("В ремонте"),
                                  QStringLiteral("Завершено"), QStringLiteral("Общая выручка")};
    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(30);
    grid->setVerticalSpacing(16);
    for (int i = 0; i < captions.size(); ++i) {
        QFrame* c = Style::card();
        c->setMinimumSize(300, 120);
        auto* cv = new QVBoxLayout(c);
        cv->setContentsMargins(20, 16, 20, 16);
        cv->addWidget(Style::label(captions[i], "statCaption"));
        cv->addStretch(1);
        QLabel* val = Style::label(QStringLiteral("0"), "statValue");
        cv->addWidget(val);
        m_values << val;
        grid->addWidget(c, i / 2, i % 2);
    }
    grid->setColumnStretch(2, 1);
    v->addLayout(grid);

    // ----- таблица выполненных заказ-нарядов -----
    v->addWidget(Style::label(QStringLiteral("Выполненные заказ-наряды"), "h2"));
    m_table = Style::table({QStringLiteral("№ заказа"), QStringLiteral("Клиент"),
                            QStringLiteral("Автомобиль"), QStringLiteral("Статус")});
    m_table->setMaximumWidth(820);
    auto* tr = new QHBoxLayout;
    tr->addWidget(m_table, 1);
    tr->addStretch(1);
    v->addLayout(tr);
    v->addStretch(1);

    connect(gen, &QPushButton::clicked, this, [this] { generate(); });
}

void ReportsPage::prepare(int) {
    // по умолчанию — последние 30 дней
    m_to->setDate(QDate::currentDate());
    m_from->setDate(QDate::currentDate().addDays(-30));
    refresh();
}

void ReportsPage::refresh() {
    if (m_from->date() > m_to->date()) {
        Msg::warn(this, QStringLiteral("Период отчёта"),
                  QStringLiteral("Дата начала не может быть позже даты окончания."));
        return;
    }
    const QString from = m_from->date().toString(Qt::ISODate);
    const QString to = m_to->date().toString(Qt::ISODate);

    int total = 0, done = 0, inWork = 0;
    double revenue = 0;
    QList<OrderRow> doneRows;
    for (const OrderRow& r : Repo::orders()) {
        if (r.dateIso < from || r.dateIso > to) continue;
        ++total;
        if (r.status == Status::Ready || r.status == Status::Issued) {
            ++done;
            revenue += r.total;
            doneRows << r;
        } else if (r.status == Status::InWork) {
            ++inWork;
        }
    }
    // порядок как в подписях карточек: Всего заказов / В ремонте / Завершено / Выручка
    m_values[0]->setText(QString::number(total));
    m_values[1]->setText(QString::number(inWork));
    m_values[2]->setText(QString::number(done));
    m_values[3]->setText(Fmt::money(revenue));

    m_table->setRowCount(int(doneRows.size()));
    for (int i = 0; i < doneRows.size(); ++i) {
        const OrderRow& r = doneRows[i];
        Style::setText(m_table, i, 0, r.number, r.id);
        Style::setText(m_table, i, 1, Fmt::shortName(r.client));
        Style::setText(m_table, i, 2, r.car);
        Style::setText(m_table, i, 3, r.status);
    }
    Style::fitTable(m_table, 320);
}

// «Сформировать отчёт»: обновляем данные на экране и сохраняем их в PDF
void ReportsPage::generate() {
    if (m_from->date() > m_to->date()) {
        refresh();                    // покажет предупреждение о неверном периоде
        return;
    }
    refresh();

    const QString period = QStringLiteral("%1 — %2")
        .arg(m_from->date().toString(QStringLiteral("dd.MM.yyyy")),
             m_to->date().toString(QStringLiteral("dd.MM.yyyy")));

    const QStringList captions = {QStringLiteral("Всего заказов"), QStringLiteral("В ремонте"),
                                  QStringLiteral("Завершено"), QStringLiteral("Общая выручка")};
    QString html = QStringLiteral("<h2>Отчёт за период %1</h2><table cellpadding='4'>").arg(period);
    for (int i = 0; i < captions.size() && i < m_values.size(); ++i)
        html += QStringLiteral("<tr><td>%1:</td><td><b>%2</b></td></tr>")
                    .arg(captions[i].toHtmlEscaped(), m_values[i]->text().toHtmlEscaped());
    html += QStringLiteral("</table><h3>Выполненные заказ-наряды</h3>"
                           "<table border='1' cellspacing='0' cellpadding='4' width='100%'><tr>");
    for (int c = 0; c < m_table->columnCount(); ++c) {
        auto* h = m_table->horizontalHeaderItem(c);
        html += QStringLiteral("<th>%1</th>").arg(h ? h->text().toHtmlEscaped() : QString());
    }
    html += QStringLiteral("</tr>");
    for (int r = 0; r < m_table->rowCount(); ++r) {
        html += QStringLiteral("<tr>");
        for (int c = 0; c < m_table->columnCount(); ++c) {
            auto* it = m_table->item(r, c);
            html += QStringLiteral("<td>%1</td>").arg(it ? it->text().toHtmlEscaped() : QString());
        }
        html += QStringLiteral("</tr>");
    }
    html += QStringLiteral("</table>");

    QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("Сохранить отчёт"),
        QStringLiteral("otchet_%1_%2.pdf").arg(m_from->date().toString(Qt::ISODate),
                                               m_to->date().toString(Qt::ISODate)),
        QStringLiteral("PDF (*.pdf)"));
    if (path.isEmpty())
        return;                       // пользователь отменил сохранение
    if (!path.endsWith(QStringLiteral(".pdf"), Qt::CaseInsensitive))
        path += QStringLiteral(".pdf");

    QPdfWriter pdf(path);
    pdf.setPageSize(QPageSize(QPageSize::A4));
    pdf.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);
    QTextDocument doc;
    doc.setHtml(html);
    doc.print(&pdf);

    QMessageBox::information(this, QStringLiteral("Отчёт"),
                             QStringLiteral("Отчёт сохранён:\n%1").arg(path));
}
