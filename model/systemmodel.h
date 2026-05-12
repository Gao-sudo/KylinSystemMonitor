#ifndef SYSTEMMODEL_H
#define SYSTEMMODEL_H

#include <QObject>
#include <QMutex>
#include <QVector>
#include <QList>
#include <QColor>
#include <QDateTime>
#include <QStringList>

// ===================== 结构体定义（只在这里定义一次） =====================

struct CpuInfo {
    QString model;
    QString architecture;
    int cores;
    double currentUsage;
};
// 内存信息结构体
struct MemoryModule {
    QString manufacturer;
    QString partNumber;
    QString type;
    double sizeGB;
    int speedMHz;
};

// 磁盘信息结构体
struct DiskInfo {
    QString device;
    QString filesystem;
    QString mountPoint;
    QString totalSize;
    QString used;
    QString available;
    double usagePercent;
};

// 磁盘 SMART 信息结构体
struct DiskSmartInfo {
    QString device;
    bool isPassed;
    double temperature;
    int powerOnHours;
    QString healthStatus;
};

// GPU 信息结构体
struct GpuInfo {
    QString name;
    QString driver;
    double temperature;
    double usage;
    int memoryTotalMB;
    int memoryUsedMB;
};

// 内核模块信息结构体
struct KernelModuleInfo {
    QString name;
    QString size;
    bool used;
    QString depends;
};

// 服务信息结构体
struct ServiceInfo {
    QString name;
    QString description;
    QString status;
    bool enabled;
};

// 硬件节点结构
struct HardwareNode {
    QString name;
    QString type;
    int level;
    int parentId;
    QColor color;
    QString detail;
};

// 历史数据点
struct HistoryPoint {
    double cpuUsage;
    double memUsage;
    double netRx;
    double netTx;
    double diskRead;
    double diskWrite;
    QDateTime timestamp;
};

class SystemModel : public QObject
{
    Q_OBJECT
public:
    static SystemModel* instance();

    // Getters
    double cpuUsage() const;
    qulonglong memTotal() const;
    qulonglong memAvail() const;
    double diskRead() const;
    double diskWrite() const;
    double netRx() const;
    double netTx() const;

    // 历史数据 Getters
    QVector<HistoryPoint> getHistory(int count = 60) const;
    QVector<double> getCpuHistory(int count = 60) const;
    QVector<double> getMemHistory(int count = 60) const;
    QVector<double> getNetRxHistory(int count = 60) const;
    QVector<double> getNetTxHistory(int count = 60) const;
    QVector<double> getDiskReadHistory(int count = 60) const;
    QVector<double> getDiskWriteHistory(int count = 60) const;

    // 添加历史数据点
    void addHistoryPoint(double cpu, double mem, double netRx, double netTx,
                         double diskRead, double diskWrite);

    // 拓扑图相关
    QVector<HardwareNode> getHardwareNodes() const;
    void discoverHardware();

    // Setters
    void setCpuUsage(double val);
    void setMemInfo(qulonglong total, qulonglong avail);
    void setDiskSpeed(double read, double write);
    void setNetSpeed(double rx, double tx);

    // 硬件信息获取
    QList<MemoryModule> getMemoryModules() const;
    QList<DiskInfo> getDiskInfo() const;
    QList<DiskSmartInfo> getDiskSmartInfo() const;
    QList<GpuInfo> getGpuInfo() const;

    // 软件信息获取
    QString getKernelVersion() const;
    QString getOsName() const;
    int getSystemUptime() const;
    double getSystemLoad() const;
    QStringList getLoadedModules() const;
    QList<KernelModuleInfo> getKernelModules() const;
    QList<ServiceInfo> getSystemServices() const;

private:
    explicit SystemModel(QObject *parent = nullptr);

    static SystemModel *s_instance;
    static QMutex s_mutex;

    mutable QMutex m_mutex;
    mutable QMutex m_hardwareMutex;
    mutable QMutex m_softwareMutex;

    double m_cpuUsage;
    qulonglong m_memTotal, m_memAvail;
    double m_diskRead, m_diskWrite;
    double m_netRx, m_netTx;

    QVector<HistoryPoint> m_history;
    static const int MAX_HISTORY = 3600;

    QVector<HardwareNode> m_hardwareNodes;

    QList<MemoryModule> m_memoryModules;
    QList<DiskSmartInfo> m_diskSmartInfo;
    QList<GpuInfo> m_gpuInfo;
    QList<KernelModuleInfo> m_kernelModules;
    QList<ServiceInfo> m_systemServices;

    void parseCPUTopology();
    void parseMemoryTopology();
    void parsePCITopology();
    void parseUSBTopology();
    void parseDiskTopology();
    void parseNetworkTopology();
};

#endif
