#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include "../controller/systemcontroller.h"
#include "../controller/exportcontroller.h"

class SystemOverviewWidget;
class ProcessManagerWidget;
class ResourceMonitorWidget;
class FileSystemMonitor;
class DataExportWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUI();
    void bindViews();

    QTabWidget *m_tabWidget;
    SystemController& m_controller;
    ExportController* m_exporter;
    SystemOverviewWidget *m_systemView;
    ProcessManagerWidget *m_processView;
    ResourceMonitorWidget *m_resourceView;
    FileSystemMonitor *m_fileSystemView;
    DataExportWidget *m_exportView;

    // Cgroup 控制函数
    void setProcessCgroupCpu(int pid, int cpuPercent);
    void removeProcessCgroupCpu(int pid);
    void setProcessCgroupMemory(int pid, int memMB);
    void removeProcessCgroupMemory(int pid);
};

#endif
