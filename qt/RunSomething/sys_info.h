#ifndef SYS_INFO_H
#define SYS_INFO_H

class SysInfo
{
public:
    static SysInfo& instance();

    virtual ~SysInfo();

    virtual void init() = 0;

    virtual double cpuUsageAverage() = 0;

    virtual double memoryUsage() = 0;

protected:
    explicit SysInfo();

private:
    SysInfo(const SysInfo& rhs);
    SysInfo& operator=(const SysInfo& rhs);
};
#endif // SYS_INFO_H
