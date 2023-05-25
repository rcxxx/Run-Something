#include "runicon.h"

RunIcon::RunIcon(QObject *parent) : QSystemTrayIcon(parent){
    this->setIcon(QIcon(":/res/resources/themes/cat/dark_cat_0.ico"));
    this->setToolTip("Run Something");     // 悬停描述

    // 创建 Socket
    // UDP_socket
    udp_socket = new QUdpSocket();
    if(!udp_socket->bind(QHostAddress::AnyIPv4, UDP_PORT)) {
        qCritical() << "Failed to bind UDP socket:" << udp_socket->errorString();
        return;
    }
    qDebug() << "UDP server started on port: " << udp_socket->localPort();
    QObject::connect(udp_socket, &QUdpSocket::readyRead, this, &RunIcon::onUDPReadyRead);

    // TCP_socket
    tcp_server = new QTcpServer();
    tcp_server->listen(QHostAddress::AnyIPv4, TCP_PORT);
    qDebug() << "TCP Server Listening on port: " << TCP_PORT;
    QObject::connect(tcp_server, &QTcpServer::newConnection, this, &RunIcon::onTCPNewConnection);

    // 创建右键菜单
    QMenu *menu = new QMenu();

    // 添加切换图标子菜单项
    QMenu *sub_menu_runner = menu->addMenu("Runner");

    cat_action = new QAction("cat", sub_menu_runner);
    cat_action->setCheckable(true);
    cat_action->setChecked(true);   // Defult
    sub_menu_runner->addAction(cat_action);

    parrot_action = new QAction("parrot", sub_menu_runner);
    parrot_action->setCheckable(true);
    parrot_action->setChecked(false);
    sub_menu_runner->addAction(parrot_action);

    // 切换图标
    QObject::connect(cat_action, &QAction::triggered, this, &RunIcon::onSubCatActionTriggered);
    QObject::connect(parrot_action, &QAction::triggered, this, &RunIcon::onSubParrotActionTriggered);

    // 添加切换主题子菜单项
    QMenu *sub_menu_theme = menu->addMenu("Themes");

    light_theme_action = new QAction("Light", sub_menu_theme);
    sub_menu_theme->addAction(light_theme_action);
    light_theme_action->setCheckable(true);
    light_theme_action->setChecked(false);

    dark_theme_action = new QAction("Dark", sub_menu_theme);
    sub_menu_theme->addAction(dark_theme_action);
    dark_theme_action->setCheckable(true);
    dark_theme_action->setChecked(true);   // Defult

    // 切换主题 亮/暗
    QObject::connect(light_theme_action, &QAction::triggered, this, &RunIcon::onSubLightThemeActionTriggered);
    QObject::connect(dark_theme_action, &QAction::triggered, this, &RunIcon::onSubDarkThemeActionTriggered);

    // 动态图标
    timer_monitor = new QTimer(this);
    timer_monitor->setInterval(monitor_interval);
    timer_animation = new QTimer(this);
    timer_animation->setInterval(animation_interval);

    QObject::connect(timer_monitor, &QTimer::timeout, this, &RunIcon::onTimerMonitorTimeout);
    QObject::connect(timer_animation, &QTimer::timeout, this, &RunIcon::onTimerAnimationTimeout);

    timer_monitor->start();
    timer_animation->start();

    // 双击打开任务管理器
    QObject::connect(this, &QSystemTrayIcon::activated, this, [&](QSystemTrayIcon::ActivationReason reason){
        if (reason == QSystemTrayIcon::DoubleClick) {
            // 打开 Windows 任务管理器
            QString program = "taskmgr";
            wchar_t* wideProgram = new wchar_t[program.size() + 1];
            program.toWCharArray(wideProgram);
            wideProgram[program.size()] = 0;
            ShellExecuteW(NULL, L"open", wideProgram, NULL, NULL, SW_SHOWDEFAULT);
            delete[] wideProgram;
        }
    });

    // 添加退出菜单项
    QAction *quit_action = new QAction("Exit", menu);
    QObject::connect(quit_action, &QAction::triggered, this, &QApplication::quit);
    menu->addAction(quit_action);

    this->setContextMenu(menu);
    this->show();
}

double RunIcon::getCpuUsage() {
    // 获取系统时间
    FILETIME idle_time, kernel_time, user_time;
    if (!GetSystemTimes(&idle_time, &kernel_time, &user_time)) {
        return -1;
    }

    // 计算 CPU 占用率
    static ULONGLONG last_idle_time = 0;
    static ULONGLONG last_kernel_time = 0;
    static ULONGLONG last_user_time = 0;
    ULONGLONG idle_time_delta = idle_time.dwLowDateTime + ((ULONGLONG)idle_time.dwHighDateTime << 32) - last_idle_time;
    ULONGLONG kernel_time_delta = kernel_time.dwLowDateTime + ((ULONGLONG)kernel_time.dwHighDateTime << 32) - last_kernel_time;
    ULONGLONG user_time_delta = user_time.dwLowDateTime + ((ULONGLONG)user_time.dwHighDateTime << 32) - last_user_time;
    last_idle_time = idle_time.dwLowDateTime + ((ULONGLONG)idle_time.dwHighDateTime << 32);
    last_kernel_time = kernel_time.dwLowDateTime + ((ULONGLONG)kernel_time.dwHighDateTime << 32);
    last_user_time = user_time.dwLowDateTime + ((ULONGLONG)user_time.dwHighDateTime << 32);

    ULONGLONG total_time_delta = kernel_time_delta + user_time_delta;
    return static_cast<double>(total_time_delta - idle_time_delta) / static_cast<double>(total_time_delta) * 100.0;
}

unsigned long RunIcon::getMemoryUsage() {
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof (mem_status);
    if (!GlobalMemoryStatusEx(&mem_status)) {
       return 0.0;
    }

    return mem_status.dwMemoryLoad;
}

void RunIcon::onSubCatActionTriggered()
{
    cat_action->setChecked(true);
    parrot_action->setChecked(false);
    icon_name = QString("cat");
    icon_num_max = icon_cat;
}

void RunIcon::onSubParrotActionTriggered()
{
    cat_action->setChecked(false);
    parrot_action->setChecked(true);
    icon_name = QString("parrot");
    icon_num_max = icon_parrot;
}

void RunIcon::onSubLightThemeActionTriggered()
{
    light_theme_action->setChecked(true);
    dark_theme_action->setChecked(false);
    theme_status = QString("light");
}

void RunIcon::onSubDarkThemeActionTriggered()
{
    light_theme_action->setChecked(false);
    dark_theme_action->setChecked(true);
    theme_status = QString("dark");
}

void RunIcon::onTimerMonitorTimeout()
{
    double cpu_usage = getCpuUsage();
    unsigned long mem_usage = getMemoryUsage();
    QString info = QString("CPU: %1%\n内存: %2%").arg(QString::number(cpu_usage,'f',2), QString::number(mem_usage));
    this->setToolTip(info);     // 悬停描述
    animation_interval = 200 - (2*round(cpu_usage));

    QByteArray send_msg = QString("CPU Usage: %1%  Memory Usage: %2%").arg(QString::number(cpu_usage,'f',2), QString::number(mem_usage)).toLocal8Bit();

    for (QTcpSocket *socket: tcp_clients) {
        socket->write(send_msg);
        socket->flush();
        socket->waitForBytesWritten();
    }
}

void RunIcon::onTimerAnimationTimeout()
{
    QString icon_file = QString(":/res/resources/themes/%1/%2_%3_%4.ico").arg(icon_name, theme_status, icon_name).arg(icon_num);
    this->setIcon(QIcon(icon_file));
    if(++icon_num > icon_num_max){
        icon_num = 0;
    };
    timer_animation->start(animation_interval);
}

void RunIcon::onUDPReadyRead()
{
    while (udp_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udp_socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        udp_socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        QString message = QString::fromUtf8(datagram);
        if (message.startsWith("ESP32 Device")) {
            QString deviceName = message.section(':', 1, 1);
            QString ipAddress = sender.toString();
            qDebug() << "ESP32 Device: " << deviceName << "is at IP address" << ipAddress << ":" << senderPort;
            qDebug() << datagram;
        }

        QByteArray send_msg = "Hello from RunSomething";
        udp_socket->writeDatagram(send_msg, sender, senderPort);
        qDebug() << "Reply: " << send_msg << " to" << sender.toString() << ":" << senderPort;
    }
}

void RunIcon::onTCPNewConnection()
{
    QTcpSocket *tcp_client = tcp_server->nextPendingConnection();
    tcp_clients.append(tcp_client);
    qDebug() << "TCP Server: connected from: " << tcp_client->peerAddress().toString();
    connect(tcp_client, &QTcpSocket::disconnected, this, &RunIcon::onTCPDisConnected);
}

void RunIcon::onTCPDisConnected()
{
    QTcpSocket *tcp_client = qobject_cast<QTcpSocket *>(sender());
    if (tcp_client) {
        tcp_clients.removeOne(tcp_client);
        tcp_client->deleteLater();
        qDebug() << "TCP Server: Disconnected from: " << tcp_client->peerAddress().toString();
    }
}
