// systemmodel.cpp
#include "systemmodel.h"
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QSysInfo>
#include <QThread>
#include <QRegularExpression>
#include <QDebug>

SystemModel *SystemModel::s_instance = nullptr;
QMutex SystemModel::s_mutex;

SystemModel::SystemModel(QObject *parent)
    : QObject(parent)
    , m_cpuUsage(0)
    , m_memTotal(0), m_memAvail(0)
    , m_diskRead(0), m_diskWrite(0)
    , m_netRx(0), m_netTx(0)
{
    discoverHardware();
}

SystemModel* SystemModel::instance()
{
    if (!s_instance) {
        QMutexLocker locker(&s_mutex);
        if (!s_instance) {
            s_instance = new SystemModel();
        }
    }
    return s_instance;
}

// ===================== Getter =====================
double SystemModel::cpuUsage() const { QMutexLocker l(&m_mutex); return m_cpuUsage; }
qulonglong SystemModel::memTotal() const { QMutexLocker l(&m_mutex); return m_memTotal; }
qulonglong SystemModel::memAvail() const { QMutexLocker l(&m_mutex); return m_memAvail; }
double SystemModel::diskRead() const { QMutexLocker l(&m_mutex); return m_diskRead; }
double SystemModel::diskWrite() const { QMutexLocker l(&m_mutex); return m_diskWrite; }
double SystemModel::netRx() const { QMutexLocker l(&m_mutex); return m_netRx; }
double SystemModel::netTx() const { QMutexLocker l(&m_mutex); return m_netTx; }

QVector<HardwareNode> SystemModel::getHardwareNodes() const
{
    QMutexLocker l(&m_mutex);
    return m_hardwareNodes;
}

// ===================== Setters =====================
void SystemModel::setCpuUsage(double val)
{
    QMutexLocker l(&m_mutex);
    m_cpuUsage = val;
}

void SystemModel::setMemInfo(qulonglong total, qulonglong avail)
{
    QMutexLocker l(&m_mutex);
    m_memTotal = total;
    m_memAvail = avail;
}

void SystemModel::setDiskSpeed(double read, double write)
{
    QMutexLocker l(&m_mutex);
    m_diskRead = read;
    m_diskWrite = write;
}

void SystemModel::setNetSpeed(double rx, double tx)
{
    QMutexLocker l(&m_mutex);
    m_netRx = rx;
    m_netTx = tx;
}

// ===================== 历史数据 =====================
void SystemModel::addHistoryPoint(double cpu, double mem, double netRx, double netTx,
                                   double diskRead, double diskWrite)
{
    QMutexLocker l(&m_mutex);
    HistoryPoint point;
    point.cpuUsage = cpu;
    point.memUsage = mem;
    point.netRx = netRx;
    point.netTx = netTx;
    point.diskRead = diskRead;
    point.diskWrite = diskWrite;
    point.timestamp = QDateTime::currentDateTime();

    m_history.append(point);
    while (m_history.size() > MAX_HISTORY) {
        m_history.removeFirst();
    }

    qDebug() << "History added - diskRead:" << diskRead << "diskWrite:" << diskWrite;
}

QVector<double> SystemModel::getCpuHistory(int count) const
{
    QMutexLocker l(&m_mutex);
    QVector<double> result;
    int start = qMax(0, m_history.size() - count);
    for (int i = start; i < m_history.size(); ++i) {
        result.append(m_history[i].cpuUsage);
    }
    return result;
}

QVector<double> SystemModel::getMemHistory(int count) const
{
    QMutexLocker l(&m_mutex);
    QVector<double> result;
    int start = qMax(0, m_history.size() - count);
    for (int i = start; i < m_history.size(); ++i) {
        result.append(m_history[i].memUsage);
    }
    return result;
}

QVector<double> SystemModel::getNetRxHistory(int count) const
{
    QMutexLocker l(&m_mutex);
    QVector<double> result;
    int start = qMax(0, m_history.size() - count);
    for (int i = start; i < m_history.size(); ++i) {
        result.append(m_history[i].netRx);
    }
    return result;
}

QVector<double> SystemModel::getNetTxHistory(int count) const
{
    QMutexLocker l(&m_mutex);
    QVector<double> result;
    int start = qMax(0, m_history.size() - count);
    for (int i = start; i < m_history.size(); ++i) {
        result.append(m_history[i].netTx);
    }
    return result;
}

QVector<double> SystemModel::getDiskReadHistory(int count) const
{
    QMutexLocker l(&m_mutex);
    QVector<double> result;
    int start = qMax(0, m_history.size() - count);
    for (int i = start; i < m_history.size(); ++i) {
        result.append(m_history[i].diskRead);
    }
    return result;
}

QVector<double> SystemModel::getDiskWriteHistory(int count) const
{
    QMutexLocker l(&m_mutex);
    QVector<double> result;
    int start = qMax(0, m_history.size() - count);
    for (int i = start; i < m_history.size(); ++i) {
        result.append(m_history[i].diskWrite);
    }
    return result;
}

// ===================== 硬件发现 =====================
void SystemModel::discoverHardware()
{
    QMutexLocker l(&m_mutex);
    m_hardwareNodes.clear();

    HardwareNode root;
    root.name = "Computer System";
    root.type = "system";
    root.level = 0;
    root.parentId = -1;
    root.color = QColor(52, 73, 94);
    root.detail = QSysInfo::prettyProductName();
    m_hardwareNodes.append(root);

    parseCPUTopology();
    parseMemoryTopology();
    parsePCITopology();
    parseUSBTopology();
    parseDiskTopology();
    parseNetworkTopology();
}

void SystemModel::parseCPUTopology()
{
    QFile cpuInfo("/proc/cpuinfo");
    if (!cpuInfo.open(QIODevice::ReadOnly)) return;

    QString content = QString::fromUtf8(cpuInfo.readAll());
    cpuInfo.close();

    int physicalCores = QThread::idealThreadCount();

    QString cpuModel = "Unknown";
    QRegularExpression re("model name\\s+: (.+)");
    QRegularExpressionMatch match = re.match(content);
    if (match.hasMatch()) {
        cpuModel = match.captured(1).trimmed();
        if (cpuModel.length() > 30) cpuModel = cpuModel.left(30) + "...";
    }

    QString arch = QSysInfo::currentCpuArchitecture();

    HardwareNode cpu;
    cpu.name = "CPU";
    cpu.type = "cpu";
    cpu.level = 1;
    cpu.parentId = 0;
    cpu.color = QColor(46, 204, 113);
    cpu.detail = QString("%1 | %2 Cores | %3").arg(cpuModel).arg(physicalCores).arg(arch);
    m_hardwareNodes.append(cpu);

    int cpuIdx = m_hardwareNodes.size() - 1;

    for (int i = 0; i < physicalCores && i < 16; i++) {
        HardwareNode core;
        core.name = QString("Core %1").arg(i + 1);
        core.type = "core";
        core.level = 2;
        core.parentId = cpuIdx;
        core.color = QColor(129, 236, 236);
        core.detail = "Processing Unit";
        m_hardwareNodes.append(core);
    }
}

void SystemModel::parseMemoryTopology()
{
    QFile memInfo("/proc/meminfo");
    if (!memInfo.open(QIODevice::ReadOnly)) return;

    QString content = QString::fromUtf8(memInfo.readAll());
    memInfo.close();

    qulonglong totalKB = 0;
    QRegularExpression re("MemTotal:\\s+(\\d+)");
    QRegularExpressionMatch match = re.match(content);
    if (match.hasMatch()) {
        totalKB = match.captured(1).toULongLong();
    }

    double totalGB = totalKB / 1024.0 / 1024.0;

    QDir numaDir("/sys/devices/system/node");
    QStringList numaNodes = numaDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    int numaCount = 0;
    for (const QString& node : numaNodes) {
        if (node.startsWith("node")) {
            numaCount++;
        }
    }

    HardwareNode mem;
    mem.name = "Memory";
    mem.type = "memory";
    mem.level = 1;
    mem.parentId = 0;
    mem.color = QColor(241, 196, 15);
    if (numaCount > 1) {
        mem.detail = QString("%1 GB | %2 NUMA Nodes").arg(totalGB, 0, 'f', 2).arg(numaCount);
    } else {
        mem.detail = QString("%1 GB | UMA Architecture").arg(totalGB, 0, 'f', 2);
    }
    m_hardwareNodes.append(mem);
}

void SystemModel::parsePCITopology()
{
    QProcess lspci;
    lspci.start("lspci", QStringList());
    if (!lspci.waitForFinished(3000)) return;

    QString output = QString::fromUtf8(lspci.readAllStandardOutput());
    QStringList lines = output.split('\n');

    QList<HardwareNode> pcieDevices;

    for (const QString& line : lines) {
        if (line.isEmpty()) continue;

        HardwareNode device;
        device.type = "pcie";
        device.level = 2;

        if (line.contains("VGA") || line.contains("3D") || line.contains("Graphics")) {
            device.name = "GPU";
            device.color = QColor(231, 76, 60);
            device.detail = line.simplified();
        }
        else if (line.contains("Ethernet") || line.contains("Network")) {
            device.name = "Network Card";
            device.color = QColor(230, 126, 34);
            device.detail = line.simplified();
        }
        else if (line.contains("SATA") || line.contains("NVMe") || line.contains("Storage")) {
            device.name = "Storage Controller";
            device.color = QColor(155, 89, 182);
            device.detail = line.simplified();
        }
        else if (line.contains("USB")) {
            device.name = "USB Controller";
            device.color = QColor(52, 152, 219);
            device.detail = line.simplified();
        }
        else if (line.contains("Audio")) {
            device.name = "Audio Device";
            device.color = QColor(142, 68, 173);
            device.detail = line.simplified();
        }
        else {
            continue;
        }

        if (device.detail.length() > 40) {
            device.detail = device.detail.left(40) + "...";
        }

        pcieDevices.append(device);
    }

    if (!pcieDevices.isEmpty()) {
        HardwareNode pcieRoot;
        pcieRoot.name = "PCIe Bus";
        pcieRoot.type = "pcie_root";
        pcieRoot.level = 1;
        pcieRoot.parentId = 0;
        pcieRoot.color = QColor(52, 152, 219);
        pcieRoot.detail = QString("%1 Devices").arg(pcieDevices.size());
        m_hardwareNodes.append(pcieRoot);

        int pcieRootId = m_hardwareNodes.size() - 1;
        for (auto& device : pcieDevices) {
            device.parentId = pcieRootId;
            m_hardwareNodes.append(device);
        }
    }
}

void SystemModel::parseUSBTopology()
{
    QProcess lsusb;
    lsusb.start("lsusb", QStringList());
    if (!lsusb.waitForFinished(3000)) return;

    QString output = QString::fromUtf8(lsusb.readAllStandardOutput());
    QStringList lines = output.split('\n');

    QVector<HardwareNode> usbDevices;

    for (const QString& line : lines) {
        if (line.isEmpty()) continue;

        HardwareNode device;
        device.name = "USB Device";
        device.type = "usb";
        device.level = 2;
        device.color = QColor(52, 152, 219, 180);
        device.detail = line.simplified();

        if (device.detail.length() > 35) {
            device.detail = device.detail.left(35) + "...";
        }

        usbDevices.append(device);
    }

    if (!usbDevices.isEmpty()) {
        HardwareNode usbRoot;
        usbRoot.name = "USB Bus";
        usbRoot.type = "usb_root";
        usbRoot.level = 1;
        usbRoot.parentId = 0;
        usbRoot.color = QColor(52, 152, 219);
        usbRoot.detail = QString("%1 Devices").arg(usbDevices.size());
        m_hardwareNodes.append(usbRoot);

        int usbRootId = m_hardwareNodes.size() - 1;
        for (int i = 0; i < usbDevices.size() && i < 8; i++) {
            usbDevices[i].parentId = usbRootId;
            m_hardwareNodes.append(usbDevices[i]);
        }

        if (usbDevices.size() > 8) {
            HardwareNode more;
            more.name = "... etc";
            more.type = "more";
            more.level = 2;
            more.parentId = usbRootId;
            more.color = QColor(100, 100, 100);
            more.detail = QString("%1 more devices").arg(usbDevices.size() - 8);
            m_hardwareNodes.append(more);
        }
    }
}

void SystemModel::parseDiskTopology()
{
    HardwareNode diskRoot;
    diskRoot.name = "Disk Storage";
    diskRoot.type = "disk_root";
    diskRoot.level = 1;
    diskRoot.parentId = 0;
    diskRoot.color = QColor(155, 89, 182);
    diskRoot.detail = "Storage Devices";
    m_hardwareNodes.append(diskRoot);

    int diskRootId = m_hardwareNodes.size() - 1;

    QStringList mountPoints = {"/", "/home", "/boot", "/var"};
    for (const QString& mp : mountPoints) {
        QProcess dfMp;
        dfMp.start("df", QStringList() << "-h" << mp);
        if (dfMp.waitForFinished(2000)) {
            QString mpOutput = QString::fromUtf8(dfMp.readAllStandardOutput());
            QStringList mpLines = mpOutput.split('\n');
            if (mpLines.size() >= 2 && mpLines[1].contains(mp)) {
                HardwareNode disk;
                disk.name = mp;
                disk.type = "partition";
                disk.level = 2;
                disk.parentId = diskRootId;
                disk.color = QColor(155, 89, 182, 180);

                #if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
                    auto parts = mpLines[1].split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                #else
                    auto parts = mpLines[1].split(QRegularExpression("\\s+"), QString::SkipEmptyParts);
                #endif
                if (parts.size() >= 4) {
                    disk.detail = QString("%1 / Used %2").arg(parts[1]).arg(parts[2]);
                } else {
                    disk.detail = "Partition";
                }
                m_hardwareNodes.append(disk);
            }
        }
    }
}

void SystemModel::parseNetworkTopology()
{
    QDir netDir("/sys/class/net");
    QStringList interfaces = netDir.entryList(QDir::NoDotAndDotDot);

    HardwareNode netRoot;
    netRoot.name = "Network Interfaces";
    netRoot.type = "net_root";
    netRoot.level = 1;
    netRoot.parentId = 0;
    netRoot.color = QColor(230, 126, 34);
    netRoot.detail = QString("%1 Interfaces").arg(interfaces.size());
    m_hardwareNodes.append(netRoot);

    int netRootId = m_hardwareNodes.size() - 1;

    for (const QString& iface : interfaces) {
        if (iface == "lo") continue;

        HardwareNode net;
        net.name = iface;
        net.type = "interface";
        net.level = 2;
        net.parentId = netRootId;
        net.color = QColor(230, 126, 34, 180);

        QFile operstate("/sys/class/net/" + iface + "/operstate");
        if (operstate.open(QIODevice::ReadOnly)) {
            QString state = QString::fromUtf8(operstate.readAll()).trimmed();
            operstate.close();

            QFile address("/sys/class/net/" + iface + "/address");
            QString mac = "";
            if (address.open(QIODevice::ReadOnly)) {
                mac = QString::fromUtf8(address.readAll()).trimmed();
                address.close();
            }

            if (state == "up") {
                net.detail = "Connected";
                net.color = QColor(46, 204, 113);
            } else {
                net.detail = "Disconnected";
                net.color = QColor(149, 165, 166);
            }

            if (!mac.isEmpty()) {
                net.detail += " | " + mac;
            }
        } else {
            net.detail = "Network Interface";
        }

        m_hardwareNodes.append(net);
    }
}

// ===================== 硬件信息获取（默认实现） =====================

QList<MemoryModule> SystemModel::getMemoryModules() const
{
    return QList<MemoryModule>();
}

QList<DiskInfo> SystemModel::getDiskInfo() const
{
    return QList<DiskInfo>();
}

QList<DiskSmartInfo> SystemModel::getDiskSmartInfo() const
{
    QList<DiskSmartInfo> disks;
    QDir devDir("/dev");
    QStringList devices = devDir.entryList(QDir::System | QDir::NoDotAndDotDot);

    for (const QString& device : devices) {
        if (device.startsWith("sd") || device.startsWith("nvme")) {
            DiskSmartInfo info;
            info.device = "/dev/" + device;
            info.isPassed = true;
            info.temperature = 0;
            info.powerOnHours = 0;
            info.healthStatus = "Unknown";
            disks.append(info);
        }
    }
    return disks;
}

QList<GpuInfo> SystemModel::getGpuInfo() const
{
    QList<GpuInfo> gpus;
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

// ===================== 软件信息获取（默认实现） =====================

QString SystemModel::getKernelVersion() const
{
    QFile ver("/proc/version");
    if (ver.open(QIODevice::ReadOnly)) {
        return QString::fromUtf8(ver.readAll()).left(200);
    }
    return "Unknown";
}

QString SystemModel::getOsName() const
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

int SystemModel::getSystemUptime() const
{
    QFile uptime("/proc/uptime");
    if (uptime.open(QIODevice::ReadOnly)) {
        double seconds = QString::fromUtf8(uptime.readAll()).trimmed().split(' ')[0].toDouble();
        return static_cast<int>(seconds);
    }
    return 0;
}

double SystemModel::getSystemLoad() const
{
    QFile loadavg("/proc/loadavg");
    if (loadavg.open(QIODevice::ReadOnly)) {
        return QString::fromUtf8(loadavg.readAll()).split(' ')[0].toDouble();
    }
    return 0.0;
}

QStringList SystemModel::getLoadedModules() const
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

QList<KernelModuleInfo> SystemModel::getKernelModules() const
{
    QList<KernelModuleInfo> modules;
    QFile modulesFile("/proc/modules");
    if (modulesFile.open(QIODevice::ReadOnly)) {
        QString content = QString::fromUtf8(modulesFile.readAll());
        modulesFile.close();
        QStringList lines = content.split('\n');
        for (const QString& line : lines) {
            if (line.isEmpty()) continue;
            QStringList parts = line.split(' ');
            if (parts.size() >= 4) {
                KernelModuleInfo info;
                info.name = parts[0];
                info.size = parts[1];
                info.used = parts[2].toInt() > 0;
                info.depends = parts[3] != "-" ? parts[3] : "";
                modules.append(info);
            }
        }
    }
    return modules;
}

QList<ServiceInfo> SystemModel::getSystemServices() const
{
    QList<ServiceInfo> services;
    QProcess systemctl;
    systemctl.start("systemctl", QStringList() << "list-units" << "--type=service"
                    << "--state=running" << "--no-pager" << "--no-legend");
    if (systemctl.waitForFinished(5000)) {
        QString output = QString::fromUtf8(systemctl.readAllStandardOutput());
        QStringList lines = output.split('\n');
        for (const QString& line : lines) {
            if (line.isEmpty()) continue;
            QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 2) {
                ServiceInfo info;
                info.name = parts[0];
                info.description = parts[1];
                info.status = "active";
                info.enabled = true;
                services.append(info);
                if (services.size() >= 10) break;
            }
        }
    }
    return services;
}
