#ifndef RUNICON_H
#define RUNICON_H

#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QIcon>
#include <QAction>
#include <QObject>
#include <QTimer>
#include <QUdpSocket>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>

#include <cmath>

enum icon_num{
    icon_cat = 4,
    icon_parrot = 9,
};

class RunIcon : public QSystemTrayIcon
{    
public:
    RunIcon(QObject *parent = nullptr);

    /**
     * @brief 获取 CPU 占用率
     * @return
     */
    double getCpuUsage();

    /**
     * @brief 获取内存使用情况
     * @return
     */
    double getMemoryUsage();

private slots:
    void onSubCatActionTriggered();
    void onSubParrotActionTriggered();

    void onSubLightThemeActionTriggered();
    void onSubDarkThemeActionTriggered();

    void onTimerMonitorTimeout();
    void onTimerAnimationTimeout();

    void onUDPReadyRead();

    void onTCPNewConnection();
    void onTCPDisConnected();

private:
    QAction *cat_action;
    QAction *parrot_action;

    QAction *light_theme_action;
    QAction *dark_theme_action;

    QTimer *timer_monitor;
    QTimer *timer_animation;

    const int UDP_PORT = 2333;
    QUdpSocket *udp_socket;

    const int TCP_PORT = 2334;
    QTcpServer *tcp_server;
    QList<QTcpSocket *> tcp_clients;

private:
    QString icon_name = QString("cat");
    unsigned int icon_num_max = icon_cat;
    unsigned int icon_num = 0;

    QString theme_status = QString("dark");

    const int monitor_interval = 1500;
    unsigned int animation_interval = 200;
};

#endif // RUNICON_H
