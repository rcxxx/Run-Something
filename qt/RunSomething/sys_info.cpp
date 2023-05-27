#include "sys_info.h"

#include <QtGlobal>

#ifdef Q_OS_WIN
#include "sys_info_win_impl.h"
#elif defined(Q_OS_LINUX)
#include "sys_info_linux_ipml.h"
#endif

SysInfo& SysInfo::instance()
{
#ifdef Q_OS_WIN
    static SysInfoWinImpl singleton;
#elif defined(Q_OS_LINUX)
    static  SysInfoLinuxIpml singleton;
#endif
    return singleton;
}

SysInfo::SysInfo()
{

}


SysInfo::~SysInfo()
{

}
