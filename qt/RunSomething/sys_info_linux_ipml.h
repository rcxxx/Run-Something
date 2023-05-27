#ifndef SYS_INFO_LINUX_IPML_H
#define SYS_INFO_LINUX_IPML_H

#include <QtGlobal>
#include <QVector>
#include "sys_info.h"

class SysInfoLinuxIpml : public SysInfo
{
public:
    SysInfoLinuxIpml();
    void init() override;
    double memoryUsage() override;
    double cpuUsageAverage() override;

private:
    QVector<qulonglong> cpuRawData();

private:
    QVector<qulonglong> m_cpu_load_last_values;
};

#endif // SYS_INFO_LINUX_IPML_H
