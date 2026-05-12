#ifndef ASYNCMONITOR_H
#define ASYNCMONITOR_H

#include <QObject>
#include <QThread>
#include <QAtomicInt>
#include <QElapsedTimer>
#include <QMutex>

struct AsyncSystemData {
    double cpuUsage = 0.0;
    double memUsage = 0.0;
    double netRx = 0.0;
    double netTx = 0.0;
    double diskRead = 0.0;
    double diskWrite = 0.0;
    long long timestamp = 0;
};
Q_DECLARE_METATYPE(AsyncSystemData)

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
    AsyncSystemData collectData() const;

    QThread* m_workerThread;
    QAtomicInt m_running;
    int m_intervalMs;
    mutable QMutex m_stateMutex;
};

#endif
