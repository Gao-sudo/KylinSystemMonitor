#ifndef ASYNCMONITOR_H
#define ASYNCMONITOR_H

#include <QObject>
#include <QThread>
#include <QAtomicInt>
#include <QElapsedTimer>
#include <QFuture>
#include <QtConcurrent>

struct AsyncSystemData {
    double cpuUsage;
    double memUsage;
    double netRx;
    double netTx;
    double diskRead;
    double diskWrite;
    long long timestamp;
};

class AsyncMonitor : public QObject
{
    Q_OBJECT
public:
    static AsyncMonitor& instance();

    void start(int intervalMs = 500);
    void stop();

signals:
    void dataReady(const AsyncSystemData& data);

private:
    AsyncMonitor();
    ~AsyncMonitor();

    void workerLoop();
    void collectData();

    QThread* m_workerThread;
    QAtomicInt m_running;
    int m_intervalMs;
};

#endif
