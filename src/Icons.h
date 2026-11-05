#pragma once
#include <QIcon>
#include <QString>

namespace Icons {
// Иконка пункта меню: серая в обычном состоянии, синяя — когда пункт выбран.
QIcon nav(const QString& name);
QIcon plain(const QString& name, const QColor& color);
}
