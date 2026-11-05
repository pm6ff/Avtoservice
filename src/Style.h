#pragma once
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QTableWidget>
#include <QWidget>

namespace Style {
QString sheet();
QTableWidget* table(const QStringList& headers, QWidget* parent = nullptr);
void fitTable(QTableWidget* t, int maxHeight = 520);
QPushButton* button(const QString& text, const char* kind, QWidget* parent = nullptr);
QLabel* label(const QString& text, const char* objectName = nullptr, QWidget* parent = nullptr);
QFrame* card(QWidget* parent = nullptr);
void setText(QTableWidget* t, int row, int col, const QString& text, int id = -1);
int currentId(const QTableWidget* t);
}

namespace Msg {
void info(QWidget* parent, const QString& title, const QString& text);
void warn(QWidget* parent, const QString& title, const QString& text);
bool confirm(QWidget* parent, const QString& title, const QString& text);
}
