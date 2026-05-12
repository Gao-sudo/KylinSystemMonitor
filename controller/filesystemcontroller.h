#ifndef FILESYSTEMCONTROLLER_H
#define FILESYSTEMCONTROLLER_H

#include <QObject>
#include <QTimer>

class FileSystemController : public QObject
{
    Q_OBJECT
public:
    static FileSystemController* instance();

    void startRefresh(int ms = 1000);
    void stopRefresh();

    double getDiskReadSpeed() const;
    double getDiskWriteSpeed() const;
    double getDiskFragmentation() const;

signals:
    void diskSpeedUpdated(double read, double write);
    void fragmentationUpdated(double frag);

private:
    explicit FileSystemController(QObject *parent = nullptr);
    static FileSystemController* s_instance;

    void refreshDiskSpeed();
    void refreshFragmentation();

    QTimer* m_timer;
    qulonglong m_lastDiskRd;
    qulonglong m_lastDiskWr;
    double m_diskRead;
    double m_diskWrite;
    double m_fragmentation;
};

#endif
