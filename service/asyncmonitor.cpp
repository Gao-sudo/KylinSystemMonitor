#include "asyncmonitor.h"
#include <QFile>
#include <QDateTime>
#include <QDebug>

AsyncMonitor::AsyncMonitor()
    : m_workerThread(nullptr)
    , m_running(0)
    , m_intervalMs(500)
{
}

AsyncMonitor::~AsyncMonitor()
{
    stop();
}

AsyncMonitor& AsyncMonitor::instance()
{
    static AsyncMonitor instance;
    return instance;
}

void AsyncMonitor::start(int intervalMs)
{
    if (m_running.loadAcquire() == 1) return;

    m_intervalMs = intervalMs;
    m_running.storeRelease(1);

    m_workerThread = QThread::create([this]() {
        workerLoop();
    });
    m_workerThread->start();
}

void AsyncMonitor::stop()
{
    if (m_running.loadAcquire() == 0) return;

    m_running.storeRelease(0);
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait(3000);
        delete m_workerThread;
        m_workerThread = nullptr;
    }
}

void AsyncMonitor::workerLoop()
{
    QElapsedTimer timer;
    timer.start();

    while (m_running.loadAcquire() == 1) {
        qint64 start = timer.elapsed();

        // 异步并发采集数据
        QFuture<AsyncSystemData> future = QtConcurrent::run([this]() {
            AsyncSystemData data;
            collectData();
            return data;
        });

        // 等待数据采集完成
        AsyncSystemData data = future.result();

        // 发射信号到主线程
        emit dataReady(data);

        // 计算延迟并调整间隔
        qint64 elapsed = timer.elapsed() - start;
        int sleepTime = qMax(0, m_intervalMs - (int)elapsed);
        if (sleepTime > 0) {
            QThread::msleep(sleepTime);
        }
    }
}

void AsyncMonitor::collectData()
{
    // 这里实现实际的数据采集
    // CPU、内存、磁盘、网络等
}
