// filesystemmonitor.h
#ifndef FILESYSTEMMONITOR_H
#define FILESYSTEMMONITOR_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QTextBrowser>
#include <QTimer>
#include <QFileSystemWatcher>

class FileSystemMonitor : public QWidget
{
    Q_OBJECT
public:
    explicit FileSystemMonitor(QWidget *parent = nullptr);

    // 添加公共方法
    void updateDiskSpeed(double read, double write);
    void updateFragmentation(double frag);

private slots:
    void refreshDiskSpeed();
    void refreshDiskFrag();
    void onFileChanged(const QString& path);

private:
    void initUI();

    QLabel* labRead;
    QLabel* labWrite;
    QLabel* labFrag;
    QTextBrowser* logBrowser;

    QFileSystemWatcher* fileWatcher;
    QTimer* diskTimer;

    qulonglong lastRd, lastWr;
};

#endif
