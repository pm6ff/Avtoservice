#include "Style.h"
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>

QString Style::sheet() {
    return QStringLiteral(R"(
* { font-family: "Inter", "Segoe UI", "Noto Sans", "Helvetica Neue", Arial, sans-serif; font-size: 13px; color: #1f2430; }
QMainWindow, #loginRoot { background: #f5f6fa; }
QScrollArea { border: none; background: transparent; }
#viewport, QStackedWidget#stack { background: #f5f6fa; }

#sidebar { background: #ffffff; border-right: 1px solid #eceff4; }
#brand { color: #2862c8; font-size: 20px; font-weight: 700; }
#muted { color: #6b7385; font-size: 12px; }
#divider { background: #dfe3ea; max-height: 1px; min-height: 1px; border: none; }
#topbar { background: #ffffff; }
#pageTitle { font-size: 22px; font-weight: 600; }
#roleLabel { color: #6b7385; font-size: 13px; }

#h1 { font-size: 22px; font-weight: 600; }
#h2 { font-size: 18px; font-weight: 600; }
#h3 { font-size: 14px; font-weight: 600; }
#hint { color: #6b7385; font-size: 14px; }
#fieldLabel { font-weight: 600; font-size: 13px; }
#error { color: #d11a1a; }
#statCaption { color: #6b7385; font-size: 14px; }
#statValue { color: #2862c8; font-size: 30px; font-weight: 700; }
#loginTitle { font-size: 28px; font-weight: 700; }
#kvKey { color: #6b7385; }
#kvValue { font-weight: 600; font-size: 14px; }

QFrame#card { background: #ffffff; border: 1px solid #dfe3ea; border-radius: 12px; }
QFrame#loginCard { background: #ffffff; border-top: 6px solid #1e5bb8; border-radius: 12px; }

QPushButton { border: none; border-radius: 8px; padding: 10px 18px; font-weight: 600; background: #f0f1f5; color: #1f2430; }
QPushButton:hover { background: #e6e8ee; }
QPushButton:disabled { color: #9aa1af; }
QPushButton[kind="primary"] { background: #2862c8; color: #ffffff; }
QPushButton[kind="primary"]:hover { background: #1f54b0; }
QPushButton[kind="primary"]:disabled { background: #a9bde3; color: #ffffff; }
QPushButton[kind="danger"] { background: #ffffff; color: #e11d1d; border: 1px solid #e11d1d; }
QPushButton[kind="danger"]:hover { background: #fdecec; }
QPushButton[kind="dangerSoft"] { background: #fde8e8; color: #d11a1a; border: 1px solid #e11d1d; }
QPushButton[kind="dangerSoft"]:hover { background: #fbd5d5; }
QPushButton[nav="true"] { text-align: left; padding: 10px 12px; background: #efefef; color: #222222; font-size: 14px; font-weight: 500; }
QPushButton[nav="true"]:hover { background: #e6e6e6; }
QPushButton[nav="true"]:checked { background: #e3edfc; color: #2862c8; font-weight: 600; }
QPushButton[logout="true"] { background: #eceef3; color: #6b7385; font-weight: 500; padding: 12px 18px; }
QPushButton::menu-indicator { subcontrol-position: right center; subcontrol-origin: padding; right: 8px; }

QPushButton[pager="true"] { min-width: 18px; padding: 7px 12px; background: #ffffff; border: 1px solid #dfe3ea; font-weight: 500; }
QPushButton[pager="true"]:hover { background: #eef3fc; }
QPushButton[pager="true"][current="true"] { background: #2862c8; color: #ffffff; border: 1px solid #2862c8; font-weight: 600; }
QPushButton[pager="true"]:disabled { color: #b5bbc8; background: #f7f8fa; }

QLineEdit, QComboBox, QDateEdit, QSpinBox, QDoubleSpinBox {
    background: #ffffff; border: 1px solid #d5dbe5; border-radius: 6px; padding: 9px 10px; min-height: 20px;
    selection-background-color: #2862c8; selection-color: #ffffff; }
QLineEdit:focus, QComboBox:focus, QDateEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus { border: 1px solid #2862c8; }
QComboBox::drop-down, QDateEdit::drop-down { border: none; width: 24px; }
QComboBox QAbstractItemView { background: #ffffff; border: 1px solid #d5dbe5; selection-background-color: #e3edfc; selection-color: #1f2430; outline: none; }

QTableWidget { background: #ffffff; border: 1px solid #dfe3ea; gridline-color: #dfe3ea;
    selection-background-color: #e3edfc; selection-color: #1f2430; outline: none; }
QTableWidget::item { padding: 4px 10px; }
QHeaderView::section { background: #f0f2f6; border: none; border-bottom: 1px solid #dfe3ea; padding: 12px 10px; font-weight: 600; }
QTableWidget QComboBox { padding: 5px 8px; border-radius: 6px; min-height: 16px; margin: 4px 6px; background: #f6f8fb; }
QTableWidget QPushButton { padding: 6px 14px; margin: 4px 6px; font-weight: 500; }

QMenu { background: #ffffff; border: 1px solid #dfe3ea; padding: 4px; }
QMenu::item { padding: 8px 24px; border-radius: 4px; }
QMenu::item:selected { background: #e3edfc; }
QScrollBar:vertical { background: transparent; width: 10px; }
QScrollBar::handle:vertical { background: #c9cfdb; border-radius: 5px; min-height: 30px; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0; width: 0; }
)");
}

QTableWidget* Style::table(const QStringList& headers, QWidget* parent) {
    auto* t = new QTableWidget(0, headers.size(), parent);
    t->setHorizontalHeaderLabels(headers);
    t->verticalHeader()->setVisible(false);
    t->verticalHeader()->setDefaultSectionSize(48);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    t->horizontalHeader()->setHighlightSections(false);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setFocusPolicy(Qt::StrongFocus);
    t->setWordWrap(true);
    t->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    return t;
}

void Style::fitTable(QTableWidget* t, int maxHeight) {
    const int header = t->horizontalHeader()->sizeHint().height();
    const int rows = qMax(1, t->rowCount());
    const int h = header + rows * t->verticalHeader()->defaultSectionSize() + 4;
    t->setFixedHeight(qMin(h, maxHeight));
}

QPushButton* Style::button(const QString& text, const char* kind, QWidget* parent) {
    auto* b = new QPushButton(text, parent);
    b->setProperty("kind", QString::fromLatin1(kind));
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

QLabel* Style::label(const QString& text, const char* objectName, QWidget* parent) {
    auto* l = new QLabel(text, parent);
    if (objectName) l->setObjectName(QString::fromLatin1(objectName));
    return l;
}

QFrame* Style::card(QWidget* parent) {
    auto* f = new QFrame(parent);
    f->setObjectName(QStringLiteral("card"));
    return f;
}

void Style::setText(QTableWidget* t, int row, int col, const QString& text, int id) {
    auto* it = new QTableWidgetItem(text);
    it->setToolTip(text);
    if (id >= 0) it->setData(Qt::UserRole, id);
    t->setItem(row, col, it);
}

int Style::currentId(const QTableWidget* t) {
    const int row = t->currentRow();
    if (row < 0) return -1;
    const QTableWidgetItem* it = t->item(row, 0);
    return it ? it->data(Qt::UserRole).toInt() : -1;
}

void Msg::info(QWidget* parent, const QString& title, const QString& text) {
    QMessageBox::information(parent, title, text);
}
void Msg::warn(QWidget* parent, const QString& title, const QString& text) {
    QMessageBox::warning(parent, title, text);
}
bool Msg::confirm(QWidget* parent, const QString& title, const QString& text) {
    QMessageBox box(QMessageBox::Question, title, text, QMessageBox::NoButton, parent);
    QPushButton* yes = box.addButton(QStringLiteral("Да"), QMessageBox::YesRole);
    box.addButton(QStringLiteral("Нет"), QMessageBox::NoRole);
    box.exec();
    return box.clickedButton() == yes;
}
