#ifndef PROCESSCONTROLLER_H
#define PROCESSCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QList>
#include "../model/processmodel.h"

class ProcessController : public QObject
{
    Q_OBJECT
public:
    static ProcessController& instance();

    void startRefresh(int ms = 2000);
    void stopRefresh();

signals:
    void processDataUpdated();

private:
    ProcessController();

    QTimer* m_timer;
    QList<ProcessInfo> readSystemProcesses();  // 直接使用 ProcessInfo，不需要 ProcessModel:: 前缀
    void refreshProcesses();
};

#endif
