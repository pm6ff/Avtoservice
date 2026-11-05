#include "Icons.h"
#include <QColor>
#include <QHash>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

static QString shapes(const QString& name) {
    static const QHash<QString, QString> s = {
        {"home", "<path d='M3 11l9-8 9 8v10a1 1 0 0 1-1 1h-5v-7H9v7H4a1 1 0 0 1-1-1z'/>"},
        {"orders", "<rect x='4' y='4' width='16' height='16' rx='1'/><path d='M4 9h16M4 14h16M4 19h16'/>"},
        {"client", "<circle cx='12' cy='8' r='4'/><path d='M4 21c0-4.4 3.6-8 8-8s8 3.6 8 8'/>"},
        {"car", "<path d='M3 13l2-6a2 2 0 0 1 2-1.4h10a2 2 0 0 1 2 1.4l2 6v5H3z'/><circle cx='7.5' cy='15' r='1'/><circle cx='16.5' cy='15' r='1'/>"},
        {"wrench", "<path d='M14.7 6.3a4 4 0 0 0-5.4 5.1L3 17.7 6.3 21l6.3-6.3a4 4 0 0 0 5.1-5.4l-2.6 2.6-2.4-.6-.6-2.4z'/>"},
        {"staff", "<circle cx='9' cy='8' r='3.5'/><path d='M2.5 20c0-3.6 2.9-6.5 6.5-6.5s6.5 2.9 6.5 6.5'/><circle cx='17' cy='9' r='2.5'/><path d='M17 14c2.8 0 5 2.2 5 5'/>"},
        {"report", "<path d='M6 3h8l5 5v13H6z'/><path d='M14 3v5h5'/>"},
        {"shield", "<path d='M12 3l8 3v6c0 5-3.5 8-8 9-4.5-1-8-4-8-9V6z'/>"},
        {"logout", "<path d='M10 4H5v16h5M14 8l4 4-4 4M18 12H9'/>"},
    };
    return s.value(name);
}

static QPixmap render(const QString& name, const QColor& color) {
    const QString svg =
        QStringLiteral("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' "
                       "stroke='%1' stroke-width='1.8' stroke-linecap='round' stroke-linejoin='round'>%2</svg>")
            .arg(color.name(), shapes(name));
    QSvgRenderer r(svg.toUtf8());
    QPixmap pm(48, 48);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    r.render(&p);
    return pm;
}

QIcon Icons::plain(const QString& name, const QColor& color) {
    return QIcon(render(name, color));
}

QIcon Icons::nav(const QString& name) {
    QIcon icon;
    icon.addPixmap(render(name, QColor(QStringLiteral("#5b6475"))), QIcon::Normal, QIcon::Off);
    icon.addPixmap(render(name, QColor(QStringLiteral("#2862c8"))), QIcon::Normal, QIcon::On);
    return icon;
}
