#include "processcontroller.h"
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QThread>
#include <QRegularExpression>
#include <QDebug>
#include <algorithm>

ProcessController::ProcessController()
    : m_timer(nullptr)
{
}

ProcessController& ProcessController::instance()
{
    static ProcessController instance;
    return instance;
}

void ProcessController::startRefresh(int ms)
{
    if (m_timer) {
        m_timer->stop();
        delete m_timer;
    }
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ProcessController::refreshProcesses);
    m_timer->start(ms);
    refreshProcesses();
}

void ProcessController::stopRefresh()
{
    if (m_timer) {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;
    }
}

void ProcessController::refreshProcesses()
{
    qDebug() << "Refreshing processes...";
    auto list = readSystemProcesses();
    qDebug() << "Found" << list.size() << "processes";
    ProcessModel::instance()->updateProcessList(list);
    emit processDataUpdated();
}

QList<ProcessInfo> ProcessController::readSystemProcesses()
{
    QList<ProcessInfo> list;
    QDir procDir("/proc");
    QStringList pids = procDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    qDebug() << "=== Reading system processes ===";

    // ========== 第一遍：读取 CPU 总时间 ==========
    unsigned long long total1 = 0;
    QFile statFile("/proc/stat");
    if (statFile.open(QIODevice::ReadOnly)) {
        QString line = statFile.readLine();
        statFile.close();
        QStringList parts = line.split(" ", Qt::SkipEmptyParts);
        for (int i = 1; i < parts.size(); ++i) {
            total1 += parts[i].toULongLong();
        }
    }
    qDebug() << "CPU total1:" << total1;

    // ========== 记录第一次进程 CPU 时间 ==========
    QMap<int, unsigned long long> firstUtime, firstStime;
    for (const QString& pidStr : pids) {
        bool ok;
        int pid = pidStr.toInt(&ok);
        if (!ok) continue;

        QFile f("/proc/" + pidStr + "/stat");
        if (f.open(QIODevice::ReadOnly)) {
            QString line = f.readLine();
            f.close();
            QStringList parts = line.split(" ", Qt::SkipEmptyParts);
            if (parts.size() >= 15) {
                firstUtime[pid] = parts[13].toULongLong();
                firstStime[pid] = parts[14].toULongLong();
            }
        }
    }

    // 短暂延时
    QThread::msleep(100);

    // ========== 第二遍：读取 CPU 总时间 ==========
    unsigned long long total2 = 0;
    if (statFile.open(QIODevice::ReadOnly)) {
        QString line = statFile.readLine();
        statFile.close();
        QStringList parts = line.split(" ", Qt::SkipEmptyParts);
        for (int i = 1; i < parts.size(); ++i) {
            total2 += parts[i].toULongLong();
        }
    }
    unsigned long long deltaTotal = total2 - total1;
    qDebug() << "CPU total2:" << total2 << "deltaTotal:" << deltaTotal;

    // ========== 遍历所有进程获取详细信息 ==========
    int processCount = 0;
    for (const QString& pidStr : pids) {
        bool ok;
        int pid = pidStr.toInt(&ok);
        if (!ok) continue;

        ProcessInfo info;
        info.pid = pid;
        info.ipcInfo = "-";

        // 读取 stat 文件
        QFile statFile2("/proc/" + pidStr + "/stat");
        if (!statFile2.open(QIODevice::ReadOnly)) continue;

        QString stat = statFile2.readLine();
        statFile2.close();
        QStringList parts = stat.split(" ", Qt::SkipEmptyParts);

        if (parts.size() >= 24) {
            info.ppid = parts[3].toInt();
            info.state = parts[2];
            info.nice = parts[18].toInt();
            info.threads = parts[19].toInt();

            // 计算 CPU 使用率
            if (firstUtime.contains(pid) && firstStime.contains(pid) && deltaTotal > 0) {
                unsigned long long utime = parts[13].toULongLong();
                unsigned long long stime = parts[14].toULongLong();
                unsigned long long deltaProc = (utime - firstUtime[pid]) + (stime - firstStime[pid]);
                info.cpu = 100.0 * deltaProc / deltaTotal;
                if (info.cpu > 100) info.cpu = 100;
                if (info.cpu < 0) info.cpu = 0;
            } else {
                info.cpu = 0;
            }
        }

        // 读取进程名称
        QFile commFile("/proc/" + pidStr + "/comm");
        if (commFile.open(QIODevice::ReadOnly)) {
            info.name = QString::fromUtf8(commFile.readAll()).trimmed();
            commFile.close();
        } else {
            info.name = pidStr;
        }

        // 读取内存使用 (VmRSS)
        QFile statusFile("/proc/" + pidStr + "/status");
        if (statusFile.open(QIODevice::ReadOnly)) {
            QString status = QString::fromUtf8(statusFile.readAll());
            statusFile.close();

            QRegularExpression re("VmRSS:\\s+(\\d+)");
            auto match = re.match(status);
            if (match.hasMatch()) {
                info.mem = match.captured(1).toDouble() / 1024.0;
            } else {
                info.mem = 0;
            }
        }

        // 读取文件句柄数
        QDir fdDir("/proc/" + pidStr + "/fd");
        if (fdDir.exists()) {
            info.fdCount = fdDir.entryList(QDir::Files | QDir::NoDotAndDotDot).size();
        } else {
            info.fdCount = 0;
        }

        // ========== IPC 信息检测 ==========
        QStringList ipcTypes;
        if (fdDir.exists()) {
            QStringList fdList = fdDir.entryList(QDir::Files | QDir::NoDotAndDotDot);
            for (const QString& fd : fdList) {
                QString link = QFile::symLinkTarget(fdDir.absoluteFilePath(fd));
                if (link.contains("socket") && !ipcTypes.contains("socket")) {
                    ipcTypes << "socket";
                }
                if (link.contains("pipe") && !ipcTypes.contains("pipe")) {
                    ipcTypes << "pipe";
                }
                if (link.contains("eventfd") && !ipcTypes.contains("eventfd")) {
                    ipcTypes << "eventfd";
                }
                if (link.contains("signalfd") && !ipcTypes.contains("signalfd")) {
                    ipcTypes << "signalfd";
                }
                if (link.contains("timerfd") && !ipcTypes.contains("timerfd")) {
                    ipcTypes << "timerfd";
                }
                // 处理方括号格式
                if (link.contains("[eventfd]") && !ipcTypes.contains("eventfd")) {
                    ipcTypes << "eventfd";
                }
                if (link.contains("[signalfd]") && !ipcTypes.contains("signalfd")) {
                    ipcTypes << "signalfd";
                }
                if (link.contains("[timerfd]") && !ipcTypes.contains("timerfd")) {
                    ipcTypes << "timerfd";
                }
                if (link.contains("anon_inode:[eventfd") && !ipcTypes.contains("eventfd")) {
                    ipcTypes << "eventfd";
                }
            }
        }
        info.ipcInfo = ipcTypes.isEmpty() ? "-" : ipcTypes.join(",");

        // 读取命令行
        QFile cmdlineFile("/proc/" + pidStr + "/cmdline");
        if (cmdlineFile.open(QIODevice::ReadOnly)) {
            QString cmd = QString::fromUtf8(cmdlineFile.readAll());
            cmdlineFile.close();
            cmd.replace(QChar(0), " ");
            info.cmdline = cmd.trimmed();
            if (!info.cmdline.isEmpty()) {
                info.name = info.cmdline.split(" ").first();
            }
        }

        list.append(info);
        processCount++;
    }

    qDebug() << "Total processes read:" << processCount;

    // 按 PID 排序
    std::sort(list.begin(), list.end(),
              [](const ProcessInfo& a, const ProcessInfo& b) {
                  return a.pid < b.pid;
              });

    return list;
}
