QT       += core gui
QT       += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

HEADERS += \
    runicon.h \ \
    sys_info.h


SOURCES += \
    main.cpp \
    runicon.cpp \
     \
    sys_info.cpp

windows {
    SOURCES += sys_info_win_impl.cpp
    HEADERS += sys_info_win_impl.h
}
linux {
    SOURCES += sys_info_linux_ipml.cpp
    HEADERS += sys_info_linux_ipml.h
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
