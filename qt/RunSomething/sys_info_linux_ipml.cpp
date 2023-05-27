#include "sys_info_linux_ipml.h"
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <QFile>

SysInfoLinuxIpml::SysInfoLinuxIpml() :
    SysInfo(),
    m_cpu_load_last_values()
{

}

void SysInfoLinuxIpml::init()
{
    m_cpu_load_last_values = cpuRawData();
}

double SysInfoLinuxIpml::memoryUsage()
{
    struct sysinfo memInfo;
    sysinfo(&memInfo);
    qulonglong totalMemory = memInfo.totalram;
    totalMemory += memInfo.totalswap;
    totalMemory *= memInfo.mem_unit;
    qulonglong totalMemoryUsed = memInfo.totalram - memInfo.freeram;
    totalMemoryUsed += memInfo.totalswap - memInfo.freeswap;
    totalMemoryUsed *= memInfo.mem_unit;
    double percent = (double)totalMemoryUsed / (double)totalMemory * 100.0;
    return qBound(0.0, percent, 100.0);
}

double SysInfoLinuxIpml::cpuUsageAverage()
{
    QVector<qulonglong> last = m_cpu_load_last_values;
    QVector<qulonglong> curr = cpuRawData();
    m_cpu_load_last_values = curr;
    double overall = (curr[0] - last[0]) + (curr[1] - last[1]) + (curr[2] - last[2]);
    //（用户时间 + 系统时间）/ 总的时间 = 占用cpu的时间，也就是占用率
    double total = overall + (curr[3] - last[3]);
    double percent = (overall / total) * 100.0;
    return qBound(0.0, percent, 100.0);
}

QVector<qulonglong> SysInfoLinuxIpml::cpuRawData()
{
    QFile file("/proc/stat");
    file.open(QIODevice::ReadOnly);
    QByteArray line = file.readLine();
    file.close();
    qulonglong totalUser = 0, totalUserNice = 0, totalSystem = 0, totalIdle = 0;
    std::sscanf(line.data(), "cpu %llu %llu %llu %llu", &totalUser, &totalUserNice, &totalSystem, &totalIdle);
    QVector<qulonglong> raw_data;
    raw_data.append(totalUser);
    raw_data.append(totalUserNice);
    raw_data.append(totalSystem);
    raw_data.append(totalIdle);
    return raw_data;
}

