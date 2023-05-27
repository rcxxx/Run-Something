#ifndef SYS_INFO_WIN_IMPL_H
#define SYS_INFO_WIN_IMPL_H

#include <QtGlobal>
#include <QVector>
#include "sys_info.h"

typedef struct _FILETIME FILETIME;

class SysInfoWinImpl : public SysInfo
{
public:
    SysInfoWinImpl();
    void init() override;

    double cpuUsageAverage() override;

    double memoryUsage() override;

};

#endif // SYS_INFO_WIN_IMPL_H
