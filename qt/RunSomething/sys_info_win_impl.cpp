#include "sys_info_win_impl.h"
#include <windows.h>

SysInfoWinImpl::SysInfoWinImpl() : SysInfo()
{

}

void SysInfoWinImpl::init()
{

}

double SysInfoWinImpl::cpuUsageAverage()
{
    // 获取系统时间
    FILETIME idle_time, kernel_time, user_time;
    if (!GetSystemTimes(&idle_time, &kernel_time, &user_time)) {
        return -1;
    }

    // 计算 CPU 占用率
    static qulonglong last_idle_time = 0;
    static qulonglong last_kernel_time = 0;
    static qulonglong last_user_time = 0;
    qulonglong idle_time_delta = idle_time.dwLowDateTime + ((qulonglong)idle_time.dwHighDateTime << 32) - last_idle_time;
    qulonglong kernel_time_delta = kernel_time.dwLowDateTime + ((qulonglong)kernel_time.dwHighDateTime << 32) - last_kernel_time;
    qulonglong user_time_delta = user_time.dwLowDateTime + ((qulonglong)user_time.dwHighDateTime << 32) - last_user_time;
    last_idle_time = idle_time.dwLowDateTime + ((qulonglong)idle_time.dwHighDateTime << 32);
    last_kernel_time = kernel_time.dwLowDateTime + ((qulonglong)kernel_time.dwHighDateTime << 32);
    last_user_time = user_time.dwLowDateTime + ((qulonglong)user_time.dwHighDateTime << 32);

    qulonglong total_time_delta = kernel_time_delta + user_time_delta;
    return static_cast<double>(total_time_delta - idle_time_delta) / static_cast<double>(total_time_delta) * 100.0;
}

double SysInfoWinImpl::memoryUsage()
{
    //获取内存的使用率
    MEMORYSTATUSEX memoryStatus;
    memoryStatus.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memoryStatus);
    qulonglong memoryPhysicalUsed = memoryStatus.ullTotalPhys - memoryStatus.ullAvailPhys;

    unsigned long mem_usage = mem_status.dwMemoryLoad;
    return (double)memoryPhysicalUsed / (double)memoryStatus.ullTotalPhys * 100.0;
}
