#ifndef TRAYICON_H
#define TRAYICON_H

#include <QWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QIcon>
#include <QAction>
#include <QObject>

class TrayIcon : public QWidget
{
    Q_OBJECT
public:
    TrayIcon();
};

#endif // TRAYICON_H
