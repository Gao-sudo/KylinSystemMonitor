#ifndef PROCESSMANAGERWIDGET_H
#define PROCESSMANAGERWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QPoint>
#include "../model/processmodel.h"

class ProcessManagerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ProcessManagerWidget(QWidget *parent = nullptr);

    QTableWidget* tableProcess() const { return m_tableProcess; }
    QTreeWidget* treeProcess() const { return m_treeProcess; }

    void updateProcessList(const QList<ProcessInfo>& processes);  // 修改这里
    void updateProcessTree(const QVector<ProcessTreeNode>& tree);

signals:
    void processRightClick(const QPoint &p);
    void killProcessRequested(int pid);
    void forceKillProcessRequested(int pid);
    void suspendProcessRequested(int pid);
    void resumeProcessRequested(int pid);
    void changePriorityRequested(int pid, int niceValue);
    void generateReportRequested(int pid, const QString& processName);
    void exportCsvRequested(int pid, const QString& processName);
    void exportJsonRequested(int pid, const QString& processName);
    void cgroupCpuRequested(int pid, int cpuPercent);
    void cgroupCpuRemoveRequested(int pid);
    void cgroupMemoryRequested(int pid, int memMB);
    void cgroupMemoryRemoveRequested(int pid);

private:
    void initUI();
    void showContextMenu(const QPoint &pos);
    void onTableRightClick(const QPoint &pos);

    QTableWidget *m_tableProcess;
    QTreeWidget *m_treeProcess;
};

#endif
