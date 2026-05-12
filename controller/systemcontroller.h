#ifndef SYSTEMCONTROLLER_H
#define SYSTEMCONTROLLER_H

#include <QObject>
#include <QTimer>
#include "../model/systemmodel.h"
#include "../model/sensormodel.h"
#include "../model/processmodel.h"

// 前向声明
struct AsyncSystemData;
class AsyncMonitor;

class SystemController : public QObject
{
    Q_OBJECT
public:
    static SystemController& instance();

    void start(int intervalMs = 1000);
    void stop();

    // 数据获取接口 - 使用 SystemModel 中已有的类型
    CpuInfo getCpuInfo();
    QVector<MemoryModule> getMemoryModules();
    QVector<DiskInfo> getDiskInfo();
    QVector<DiskSmartInfo> getDiskSmartInfo();
    QVector<GpuInfo> getGpuInfo();
    QString getKernelVersion();
    QString getOsName();
    int getSystemUptime();
    double getSystemLoad();
    QStringList getLoadedModules();

    QVector<TemperatureSensor> getTemperatures();
    QVector<VoltageSensor> getVoltages();
    QVector<FanSensor> getFans();

    QList<ProcessInfo> getProcessList();
    QVector<ProcessTreeNode> getProcessTree();

signals:
    void dataUpdated();

private slots:
    void onAsyncDataReady(const AsyncSystemData& data);
    void onTimerTimeout();

private:
    SystemController();
    void refreshAll();
    void refreshCpu();
    void refreshMemory();
    void refreshDisk();
    void refreshNetwork();

    QTimer* m_timer;
    SystemModel* m_system;
    SensorModel* m_sensor;
    ProcessModel* m_process;
    AsyncMonitor* m_asyncMonitor;

    // 磁盘速率计算缓存
    qulonglong m_lastDiskRead;
    qulonglong m_lastDiskWrite;
    QDateTime m_lastDiskTime;

    // 网络速率计算缓存
    qulonglong m_lastNetRx;
    qulonglong m_lastNetTx;
    QDateTime m_lastNetTime;

    // CPU 计算缓存
    unsigned long long m_lastTotalCpu;
    unsigned long long m_lastIdleCpu;
};

#endif
