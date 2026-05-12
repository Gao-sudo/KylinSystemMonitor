#include "systemcontroller.h"
#include "../service/asyncmonitor.h"
#include "processcontroller.h"  // 添加这行
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QSysInfo>
#include <QThread>
#include <QDateTime>
#include <QRegularExpression>
#include <QDebug>

SystemController::SystemController()
    : m_timer(nullptr)
    , m_system(nullptr)
    , m_sensor(nullptr)
    , m_process(nullptr)
    , m_asyncMonitor(nullptr)
    , m_lastDiskRead(0)
    , m_lastDiskWrite(0)
    , m_lastNetRx(0)
    , m_lastNetTx(0)
    , m_lastTotalCpu(0)
    , m_lastIdleCpu(0)
{
    m_system = SystemModel::instance();
    m_sensor = SensorModel::instance();
    m_process = ProcessModel::instance();
    m_asyncMonitor = &AsyncMonitor::instance();

    m_lastDiskTime = QDateTime::currentDateTime();
    m_lastNetTime = QDateTime::currentDateTime();
}

SystemController& SystemController::instance()
{
    static SystemController instance;
    return instance;
}

void SystemController::start(int intervalMs)
{
    if (m_timer) return;

    // 连接异步监控信号
    if (m_asyncMonitor) {
        connect(m_asyncMonitor, &AsyncMonitor::dataReady,
                this, &SystemController::onAsyncDataReady);
        m_asyncMonitor->start(intervalMs);
    }

    // 定时器作为备用
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SystemController::onTimerTimeout);
    m_timer->start(intervalMs);

    // 启动传感器监控
    if (m_sensor) {
        m_sensor->startMonitoring(2000);
    }

    // 启动进程监控
    ProcessController::instance().startRefresh(2000);

    // 立即刷新一次
    onTimerTimeout();
}

void SystemController::stop()
{
    if (m_asyncMonitor) {
        m_asyncMonitor->stop();
    }

    if (m_timer) {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;
    }

    if (m_sensor) {
        m_sensor->stopMonitoring();
    }

    ProcessController::instance().stopRefresh();
}

void SystemController::onTimerTimeout()
{
    refreshAll();
}

void SystemController::onAsyncDataReady(const AsyncSystemData& data)
{
    if (m_system) {
        m_system->setCpuUsage(data.cpuUsage);
        m_system->setDiskSpeed(data.diskRead, data.diskWrite);
        m_system->setNetSpeed(data.netRx, data.netTx);

        // 添加历史数据
        double memTotal = m_system->memTotal() / 1024.0;
        double memAvail = m_system->memAvail() / 1024.0;
        double memUsage = (memTotal > 0) ? 100.0 * (memTotal - memAvail) / memTotal : 0;
        m_system->addHistoryPoint(data.cpuUsage, memUsage, data.netRx, data.netTx,
                                   data.diskRead, data.diskWrite);
    }

    emit dataUpdated();
}

void SystemController::refreshAll()
{
    refreshCpu();
    refreshMemory();
    refreshDisk();
    refreshNetwork();

    emit dataUpdated();
}

void SystemController::refreshCpu()
{
    unsigned long long total = 0, idle = 0;
    QFile f("/proc/stat");
    if (f.open(QIODevice::ReadOnly)) {
        QString line = f.readLine();
        f.close();
        QStringList parts = line.split(" ", Qt::SkipEmptyParts);
        if (parts.size() > 4) {
            idle = parts[4].toULongLong();
            for (int i = 1; i < parts.size(); ++i) {
                total += parts[i].toULongLong();
            }
        }
    }

    if (m_lastTotalCpu > 0 && total > m_lastTotalCpu) {
        unsigned long long diffTotal = total - m_lastTotalCpu;
        unsigned long long diffIdle = idle - m_lastIdleCpu;
        double cpu = 100.0 * (diffTotal - diffIdle) / diffTotal;
        if (cpu < 0) cpu = 0;
        if (cpu > 100) cpu = 100;

        if (m_system) {
            m_system->setCpuUsage(cpu);
        }
    }

    m_lastTotalCpu = total;
    m_lastIdleCpu = idle;
}

void SystemController::refreshMemory()
{
    qulonglong total = 0, avail = 0;
    QFile f("/proc/meminfo");
    if (f.open(QIODevice::ReadOnly)) {
        QString content = QString::fromUtf8(f.readAll());
        f.close();
        QStringList lines = content.split("\n");
        for (const QString& line : lines) {
            if (line.startsWith("MemTotal:")) {
                QStringList parts = line.split(" ", Qt::SkipEmptyParts);
                if (parts.size() > 1) total = parts[1].toULongLong();
            }
            if (line.startsWith("MemAvailable:")) {
                QStringList parts = line.split(" ", Qt::SkipEmptyParts);
                if (parts.size() > 1) avail = parts[1].toULongLong();
            }
        }
    }

    if (m_system) {
        m_system->setMemInfo(total, avail);
    }
}

void SystemController::refreshDisk()
{
    qulonglong readBytes = 0, writeBytes = 0;
    QFile f("/proc/diskstats");
    if (f.open(QIODevice::ReadOnly)) {
        QString content = QString::fromUtf8(f.readAll());
        f.close();
        QStringList lines = content.split("\n");
        for (const QString& line : lines) {
            if (line.contains("sda") || line.contains("nvme") || line.contains("vda")) {
                QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                if (parts.size() >= 14) {
                    readBytes += parts[5].toULongLong() * 512;
                    writeBytes += parts[9].toULongLong() * 512;
                    break;
                }
            }
        }
    }

    double readSpeed = 0, writeSpeed = 0;
    QDateTime now = QDateTime::currentDateTime();
    if (m_lastDiskRead > 0 && m_lastDiskTime.isValid()) {
        qint64 elapsedMs = m_lastDiskTime.msecsTo(now);
        if (elapsedMs > 0) {
            readSpeed = (readBytes - m_lastDiskRead) / (1024.0 * 1024.0) / (elapsedMs / 1000.0);
            writeSpeed = (writeBytes - m_lastDiskWrite) / (1024.0 * 1024.0) / (elapsedMs / 1000.0);
            if (readSpeed < 0) readSpeed = 0;
            if (writeSpeed < 0) writeSpeed = 0;
        }
    }

    m_lastDiskRead = readBytes;
    m_lastDiskWrite = writeBytes;
    m_lastDiskTime = now;

    if (m_system) {
        m_system->setDiskSpeed(readSpeed, writeSpeed);
    }
}

void SystemController::refreshNetwork()
{
    qulonglong rx = 0, tx = 0;
    QFile f("/proc/net/dev");
    if (f.open(QIODevice::ReadOnly)) {
        QString content = QString::fromUtf8(f.readAll());
        f.close();
        QStringList lines = content.split("\n");
        for (const QString& line : lines) {
            if (line.contains("eno") || line.contains("eth") || line.contains("enp") ||
                line.contains("wlan") || line.contains("wlp")) {
                QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                if (parts.size() >= 10) {
                    QString iface = parts[0];
                    if (iface.endsWith(":")) iface.chop(1);
                    if (iface != "lo") {
                        rx += parts[1].toULongLong();
                        tx += parts[9].toULongLong();
                    }
                }
            }
        }
    }

    double rxSpeed = 0, txSpeed = 0;
    QDateTime now = QDateTime::currentDateTime();
    if (m_lastNetRx > 0 && m_lastNetTime.isValid()) {
        qint64 elapsedMs = m_lastNetTime.msecsTo(now);
        if (elapsedMs > 0) {
            rxSpeed = (rx - m_lastNetRx) / (1024.0 * 1024.0) / (elapsedMs / 1000.0);
            txSpeed = (tx - m_lastNetTx) / (1024.0 * 1024.0) / (elapsedMs / 1000.0);
            if (rxSpeed < 0) rxSpeed = 0;
            if (txSpeed < 0) txSpeed = 0;
        }
    }

    m_lastNetRx = rx;
    m_lastNetTx = tx;
    m_lastNetTime = now;

    if (m_system) {
        m_system->setNetSpeed(rxSpeed, txSpeed);
    }
}

// ===================== 数据获取接口实现 =====================

CpuInfo SystemController::getCpuInfo()
{
    CpuInfo info;
    info.cores = QThread::idealThreadCount();
    info.architecture = QSysInfo::currentCpuArchitecture();

    if (m_system) {
        info.currentUsage = m_system->cpuUsage();
    }

    QFile cpuinfo("/proc/cpuinfo");
    if (cpuinfo.open(QIODevice::ReadOnly)) {
        QString content = QString::fromUtf8(cpuinfo.readAll());
        cpuinfo.close();
        QStringList lines = content.split('\n');
        for (const QString& line : lines) {
            if (line.startsWith("model name")) {
                info.model = line.split(":").value(1).trimmed();
                break;
            }
        }
    }

    return info;
}

QVector<MemoryModule> SystemController::getMemoryModules()
{
    QVector<MemoryModule> modules;
    return modules;
}

QVector<DiskInfo> SystemController::getDiskInfo()
{
    QVector<DiskInfo> disks;
    QProcess df;
    df.start("df", QStringList() << "-h");
    if (df.waitForFinished(2000)) {
        QString output = QString::fromUtf8(df.readAllStandardOutput());
        QStringList lines = output.split('\n');
        for (int i = 1; i < lines.size(); ++i) {
            QStringList parts = lines[i].split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 6) {
                DiskInfo disk;
                disk.filesystem = parts[0];
                disk.totalSize = parts[1];
                disk.used = parts[2];
                disk.available = parts[3];
                disk.usagePercent = parts[4].remove("%").toDouble();
                disk.mountPoint = parts[5];
                disks.append(disk);
            }
        }
    }
    return disks;
}

QVector<DiskSmartInfo> SystemController::getDiskSmartInfo()
{
    QVector<DiskSmartInfo> disks;
    return disks;
}

QVector<GpuInfo> SystemController::getGpuInfo()
{
    QVector<GpuInfo> gpus;

    QProcess lspci;
    lspci.start("lspci", QStringList() << "|" << "grep" << "-i" << "vga");
    if (lspci.waitForFinished(2000)) {
        QString output = QString::fromUtf8(lspci.readAllStandardOutput()).trimmed();
        if (!output.isEmpty()) {
            GpuInfo info;
            info.name = output;
            info.driver = "Unknown";
            info.temperature = 0;
            info.usage = 0;
            info.memoryTotalMB = 0;
            info.memoryUsedMB = 0;
            gpus.append(info);
        }
    }

    return gpus;
}

QString SystemController::getKernelVersion()
{
    QFile ver("/proc/version");
    if (ver.open(QIODevice::ReadOnly)) {
        return QString::fromUtf8(ver.readAll()).left(150);
    }
    return "Unknown";
}

QString SystemController::getOsName()
{
    QFile osRelease("/etc/os-release");
    if (osRelease.open(QIODevice::ReadOnly)) {
        QString content = QString::fromUtf8(osRelease.readAll());
        QStringList lines = content.split('\n');
        for (const QString& line : lines) {
            if (line.startsWith("PRETTY_NAME=")) {
                QString name = line.split("=").value(1);
                name.remove("\"");
                return name;
            }
        }
    }
    return "Unknown";
}

int SystemController::getSystemUptime()
{
    QFile uptime("/proc/uptime");
    if (uptime.open(QIODevice::ReadOnly)) {
        double seconds = QString::fromUtf8(uptime.readAll()).trimmed().split(' ')[0].toDouble();
        return static_cast<int>(seconds);
    }
    return 0;
}

double SystemController::getSystemLoad()
{
    QFile loadavg("/proc/loadavg");
    if (loadavg.open(QIODevice::ReadOnly)) {
        return QString::fromUtf8(loadavg.readAll()).split(' ')[0].toDouble();
    }
    return 0.0;
}

QStringList SystemController::getLoadedModules()
{
    QStringList modules;
    QFile modulesFile("/proc/modules");
    if (modulesFile.open(QIODevice::ReadOnly)) {
        QString content = QString::fromUtf8(modulesFile.readAll());
        modulesFile.close();
        QStringList lines = content.split('\n');
        for (int i = 0; i < lines.size() && i < 30; ++i) {
            if (!lines[i].isEmpty()) {
                modules.append(lines[i].split(' ')[0]);
            }
        }
    }
    return modules;
}

QVector<TemperatureSensor> SystemController::getTemperatures()
{
    if (m_sensor) {
        return m_sensor->getTemperatures();
    }
    return QVector<TemperatureSensor>();
}

QVector<VoltageSensor> SystemController::getVoltages()
{
    if (m_sensor) {
        return m_sensor->getVoltages();
    }
    return QVector<VoltageSensor>();
}

QVector<FanSensor> SystemController::getFans()
{
    if (m_sensor) {
        return m_sensor->getFans();
    }
    return QVector<FanSensor>();
}

QList<ProcessInfo> SystemController::getProcessList()
{
    if (m_process) {
        return m_process->getProcessList();
    }
    return QList<ProcessInfo>();
}

QVector<ProcessTreeNode> SystemController::getProcessTree()
{
    if (m_process) {
        return m_process->getProcessTree();
    }
    return QVector<ProcessTreeNode>();
}
