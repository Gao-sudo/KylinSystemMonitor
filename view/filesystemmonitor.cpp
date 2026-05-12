#include "filesystemmonitor.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QDateTime>
#include <QRegularExpression>

FileSystemMonitor::FileSystemMonitor(QWidget *parent)
    : QWidget(parent)
{
    initUI();

    fileWatcher = new QFileSystemWatcher(this);
    fileWatcher->addPath("/home");
    fileWatcher->addPath("/tmp");
    connect(fileWatcher, &QFileSystemWatcher::fileChanged, this, &FileSystemMonitor::onFileChanged);
    connect(fileWatcher, &QFileSystemWatcher::directoryChanged, this, &FileSystemMonitor::onFileChanged);

    diskTimer = new QTimer(this);
    connect(diskTimer, &QTimer::timeout, this, &FileSystemMonitor::refreshDiskSpeed);
    connect(diskTimer, &QTimer::timeout, this, &FileSystemMonitor::refreshDiskFrag);
    diskTimer->start(1000);

    lastRd = 0;
    lastWr = 0;
}

void FileSystemMonitor::initUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15,15,15,15);

    // 磁盘读写
    QGroupBox* boxSpeed = new QGroupBox("磁盘读写速率");
    QHBoxLayout* laySpeed = new QHBoxLayout(boxSpeed);
    labRead = new QLabel("读取: 0.00 MB/s");
    labWrite = new QLabel("写入: 0.00 MB/s");
    laySpeed->addWidget(labRead);
    laySpeed->addWidget(labWrite);
    mainLayout->addWidget(boxSpeed);

    // 碎片率
    QGroupBox* boxFrag = new QGroupBox("磁盘碎片率");
    QHBoxLayout* layFrag = new QHBoxLayout(boxFrag);
    labFrag = new QLabel("估计碎片率: 计算中...");
    layFrag->addWidget(labFrag);
    mainLayout->addWidget(boxFrag);

    // 文件日志
    QGroupBox* boxLog = new QGroupBox("文件操作日志追踪");
    QVBoxLayout* layLog = new QVBoxLayout(boxLog);
    logBrowser = new QTextBrowser;
    logBrowser->setStyleSheet("font-size:12px;");
    layLog->addWidget(logBrowser);
    mainLayout->addWidget(boxLog,1);
}

// 磁盘读写速率
void FileSystemMonitor::refreshDiskSpeed()
{
    qulonglong rd = 0, wr = 0;
    QFile f("/proc/diskstats");
    if (f.open(QIODevice::ReadOnly)) {
        // 修复：QByteArray → 正确转 QString
        QString content = QString::fromUtf8(f.readAll());
        QStringList lines = content.split('\n');

        for (auto& line : lines) {
            if (line.contains("sda") || line.contains("nvme") || line.contains("vd")) {
                QStringList p = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                if (p.size() >= 14) {
                    rd += p[5].toULongLong() * 512;
                    wr += p[9].toULongLong() * 512;
                }
                break;
            }
        }
        f.close();
    }

    double rSpeed = (rd - lastRd) / 1024.0 / 1024.0;
    double wSpeed = (wr - lastWr) / 1024.0 / 1024.0;
    lastRd = rd;
    lastWr = wr;

    labRead->setText(QString("读取: %1 MB/s").arg(rSpeed, 0, 'f', 2));
    labWrite->setText(QString("写入: %1 MB/s").arg(wSpeed, 0, 'f', 2));
}

// 磁盘碎片率（估算）
void FileSystemMonitor::refreshDiskFrag()
{
    QProcess p;
    p.start("df", QStringList() << "-i" << "/");
    p.waitForFinished();

    QString out = QString::fromUtf8(p.readAllStandardOutput());
    double frag = 0.0;

    QStringList lines = out.split('\n');
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

    labFrag->setText(QString("估计碎片率: %1%").arg(frag, 0, 'f', 1));
}

// 文件操作日志
void FileSystemMonitor::onFileChanged(const QString& path)
{
    QString t = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString msg = QString("[%1] 变动 → %2").arg(t, path);
    logBrowser->append(msg);

    // 限制行数
    if (logBrowser->document()->lineCount() > 100) {
        logBrowser->clear();
    }
}

void FileSystemMonitor::updateDiskSpeed(double read, double write)
{
    labRead->setText(QString("Read: %1 MB/s").arg(read, 0, 'f', 2));
    labWrite->setText(QString("Write: %1 MB/s").arg(write, 0, 'f', 2));
}

void FileSystemMonitor::updateFragmentation(double frag)
{
    labFrag->setText(QString("Fragmentation: %1%").arg(frag, 0, 'f', 1));
}
