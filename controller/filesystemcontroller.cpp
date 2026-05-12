#include "filesystemcontroller.h"
#include <QFile>
#include <QProcess>
#include <QRegularExpression>

FileSystemController* FileSystemController::s_instance = nullptr;

FileSystemController::FileSystemController(QObject *parent)
    : QObject(parent)
    , m_lastDiskRd(0)
    , m_lastDiskWr(0)
    , m_diskRead(0)
    , m_diskWrite(0)
    , m_fragmentation(0)
{
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &FileSystemController::refreshDiskSpeed);
    connect(m_timer, &QTimer::timeout, this, &FileSystemController::refreshFragmentation);
}

FileSystemController* FileSystemController::instance()
{
    if (!s_instance)
        s_instance = new FileSystemController;
    return s_instance;
}

void FileSystemController::startRefresh(int ms)
{
    m_timer->start(ms);
}

void FileSystemController::stopRefresh()
{
    m_timer->stop();
}

void FileSystemController::refreshDiskSpeed()
{
    qulonglong rd = 0, wr = 0;
    QFile f("/proc/diskstats");
    if (f.open(QIODevice::ReadOnly)) {
        QString content = QString::fromUtf8(f.readAll());
        f.close();
        QStringList lines = content.split("\n");

        for (auto& line : lines) {
            if (line.contains("sda") || line.contains("nvme") || line.contains("vda")) {
                QStringList p = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                if (p.size() >= 14) {
                    rd = p[5].toULongLong() * 512;
                    wr = p[9].toULongLong() * 512;
                    break;
                }
            }
        }
    }

    m_diskRead = (rd - m_lastDiskRd) / (1024.0 * 1024.0);
    m_diskWrite = (wr - m_lastDiskWr) / (1024.0 * 1024.0);
    if (m_diskRead < 0) m_diskRead = 0;
    if (m_diskWrite < 0) m_diskWrite = 0;

    m_lastDiskRd = rd;
    m_lastDiskWr = wr;

    emit diskSpeedUpdated(m_diskRead, m_diskWrite);
}

void FileSystemController::refreshFragmentation()
{
    QProcess p;
    p.start("df", QStringList() << "-i" << "/");
    p.waitForFinished();

    QString out = QString::fromUtf8(p.readAllStandardOutput());
    double frag = 0.0;

    QStringList lines = out.split("\n");
    for (auto& line : lines) {
        if (line.contains("/")) {
            QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 5) {
                double used = parts[2].toDouble();
                double free = parts[3].toDouble();
                if (used + free > 0)
                    frag = used / (used + free) * 100.0;
            }
            break;
        }
    }

    m_fragmentation = frag;
    emit fragmentationUpdated(m_fragmentation);
}

double FileSystemController::getDiskReadSpeed() const { return m_diskRead; }
double FileSystemController::getDiskWriteSpeed() const { return m_diskWrite; }
double FileSystemController::getDiskFragmentation() const { return m_fragmentation; }
