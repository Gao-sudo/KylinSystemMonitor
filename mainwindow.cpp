// mainwindow.cpp
#include "mainwindow.h"
#include "view/systemoverviewwidget.h"
#include "view/processmanagerwidget.h"
#include "view/resourcemonitorwidget.h"
#include "view/filesystemmonitor.h"
#include "view/dataexportwidget.h"
#include "controller/systemcontroller.h"
#include "controller/processcontroller.h"
#include "controller/exportcontroller.h"
#include "controller/filesystemcontroller.h"
#include "view/processreportdialog.h"

#include <QVBoxLayout>
#include <QStatusBar>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_tabWidget(nullptr)
    , m_controller(SystemController::instance())
    , m_exporter(nullptr)
    , m_systemView(nullptr)
    , m_processView(nullptr)
    , m_resourceView(nullptr)
    , m_fileSystemView(nullptr)
    , m_exportView(nullptr)
{
    m_exporter = &ExportController::instance();

    setupUI();
    bindViews();

    m_controller.start(1000);
    ProcessController::instance().startRefresh(2000);
}

MainWindow::~MainWindow()
{
    m_controller.stop();
    ProcessController::instance().stopRefresh();
}

void MainWindow::setupUI()
{
    setWindowTitle("Kylin System Monitor");
    setMinimumSize(1200, 800);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    m_tabWidget = new QTabWidget(this);

    m_systemView = new SystemOverviewWidget(this);
    m_processView = new ProcessManagerWidget(this);
    m_resourceView = new ResourceMonitorWidget(this);
    m_fileSystemView = new FileSystemMonitor(this);
    m_exportView = new DataExportWidget(this);

    m_tabWidget->addTab(m_systemView, "System");
    m_tabWidget->addTab(m_processView, "Process");
    m_tabWidget->addTab(m_resourceView, "Resource");
    m_tabWidget->addTab(m_fileSystemView, "File System");
    m_tabWidget->addTab(m_exportView, "Export");

    mainLayout->addWidget(m_tabWidget);
}

void MainWindow::bindViews()
{
    // ========== 系统数据更新 ==========
    connect(&m_controller, &SystemController::dataUpdated, this, [this]() {
        SystemModel* systemModel = SystemModel::instance();

        // 更新系统概览 (Tab 0)
        if (m_systemView) {
            m_systemView->updateSystemInfo(
                systemModel->cpuUsage(),
                systemModel->memTotal(),
                systemModel->memAvail()
            );
            m_systemView->updateTopology(systemModel->getHardwareNodes());
        }

        // 更新资源监控 (Tab 2)
        if (m_resourceView) {
            double memTotal = systemModel->memTotal() / 1024.0;
            double memAvail = systemModel->memAvail() / 1024.0;
            double memUsage = (memTotal > 0) ? 100.0 * (memTotal - memAvail) / memTotal : 0;

            m_resourceView->updateRealtimeData(
                systemModel->cpuUsage(),
                memUsage,
                systemModel->netRx(),
                systemModel->netTx(),
                systemModel->diskRead(),
                systemModel->diskWrite()
            );

            m_resourceView->updateCharts(
                systemModel->getCpuHistory(60),
                systemModel->getMemHistory(60),
                systemModel->getNetRxHistory(60),
                systemModel->getNetTxHistory(60),
                systemModel->getDiskReadHistory(60),
                systemModel->getDiskWriteHistory(60)
            );
        }

        // 更新文件系统监控 (Tab 3) - 磁盘速度
        if (m_fileSystemView) {
            m_fileSystemView->updateDiskSpeed(systemModel->diskRead(), systemModel->diskWrite());
        }

        // 状态栏提示
        static int cnt = 0;
        if (++cnt % 10 == 0) {
            statusBar()->showMessage("Updated: " + QDateTime::currentDateTime().toString("HH:mm:ss"), 2000);
        }
    });

    // ========== 进程数据更新 (Tab 1) ==========
    connect(&ProcessController::instance(), &ProcessController::processDataUpdated,
            this, [this]() {
        if (m_processView) {
            ProcessModel* procModel = ProcessModel::instance();
            m_processView->updateProcessList(procModel->getProcessList());
            m_processView->updateProcessTree(procModel->getProcessTree());
            qDebug() << "Process list updated, count:" << procModel->getProcessList().size();
        }
    });

    // ========== 进程控制信号连接 ==========
    if (m_processView) {
        // 1. Process Control - Kill
        connect(m_processView, &ProcessManagerWidget::killProcessRequested,
                this, [=](int pid) {
            qDebug() << "Kill requested for PID:" << pid;
            bool result = ProcessModel::instance()->killProcess(pid, 15);
            qDebug() << "Kill result:" << result;
        });

        connect(m_processView, &ProcessManagerWidget::forceKillProcessRequested,
                this, [=](int pid) {
            qDebug() << "Force kill requested for PID:" << pid;
            bool result = ProcessModel::instance()->killProcess(pid, 9);
            qDebug() << "Force kill result:" << result;
        });

        connect(m_processView, &ProcessManagerWidget::suspendProcessRequested,
                this, [=](int pid) {
            qDebug() << "Suspend requested for PID:" << pid;
            ProcessModel::instance()->killProcess(pid, 19);
        });

        connect(m_processView, &ProcessManagerWidget::resumeProcessRequested,
                this, [=](int pid) {
            qDebug() << "Resume requested for PID:" << pid;
            ProcessModel::instance()->killProcess(pid, 18);
        });

        // 2. Adjust Priority - Nice Value
        connect(m_processView, &ProcessManagerWidget::changePriorityRequested,
                this, [=](int pid, int niceValue) {
            qDebug() << "Change priority received:" << pid << niceValue;
            bool result = ProcessModel::instance()->setNice(pid, niceValue);
            if (result) {
                statusBar()->showMessage(QString("Priority changed for PID %1 to nice=%2").arg(pid).arg(niceValue), 3000);
            } else {
                statusBar()->showMessage(QString("Failed to change priority for PID %1").arg(pid), 3000);
            }
        });

        // 3. Cgroup Control
        connect(m_processView, &ProcessManagerWidget::cgroupCpuRequested,
                this, [=](int pid, int cpuPercent) {
            qDebug() << "Cgroup CPU request:" << pid << cpuPercent;
            setProcessCgroupCpu(pid, cpuPercent);
        });

        connect(m_processView, &ProcessManagerWidget::cgroupCpuRemoveRequested,
                this, [=](int pid) {
            qDebug() << "Cgroup CPU remove request:" << pid;
            removeProcessCgroupCpu(pid);
        });

        connect(m_processView, &ProcessManagerWidget::cgroupMemoryRequested,
                this, [=](int pid, int memMB) {
            qDebug() << "Cgroup memory request:" << pid << memMB;
            setProcessCgroupMemory(pid, memMB);
        });

        connect(m_processView, &ProcessManagerWidget::cgroupMemoryRemoveRequested,
                this, [=](int pid) {
            qDebug() << "Cgroup memory remove request:" << pid;
            removeProcessCgroupMemory(pid);
        });

        // 4. Resource Report
        connect(m_processView, &ProcessManagerWidget::generateReportRequested,
                this, [=](int pid, const QString& processName) {
            qDebug() << "Generate report requested - PID:" << pid << "Name:" << processName;
            ProcessReportDialog *dialog = new ProcessReportDialog(pid, processName, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            dialog->show();
        });

        // 5. Export Process Data
        connect(m_processView, &ProcessManagerWidget::exportCsvRequested,
                this, [=](int pid, const QString& processName) {
            qDebug() << "Export CSV requested for PID:" << pid;
            QString path = QFileDialog::getSaveFileName(this, "Export CSV",
                                                        QDir::homePath() + "/process_" + QString::number(pid) + ".csv",
                                                        "CSV (*.csv)");
            if (!path.isEmpty()) {
                ExportController::instance().exportProcessToCsv(pid, path);
            }
        });

        connect(m_processView, &ProcessManagerWidget::exportJsonRequested,
                this, [=](int pid, const QString& processName) {
            qDebug() << "Export JSON requested for PID:" << pid;
            QString path = QFileDialog::getSaveFileName(this, "Export JSON",
                                                        QDir::homePath() + "/process_" + QString::number(pid) + ".json",
                                                        "JSON (*.json)");
            if (!path.isEmpty()) {
                ExportController::instance().exportProcessToJson(pid, path);
            }
        });
    }

    // ========== 文件系统监控独立更新 ==========
    if (m_fileSystemView) {
        connect(FileSystemController::instance(), &FileSystemController::fragmentationUpdated,
                this, [=](double frag) {
            m_fileSystemView->updateFragmentation(frag);
        });
    }
}

void MainWindow::setProcessCgroupCpu(int pid, int cpuPercent)
{
    qDebug() << "Setting CPU cgroup for PID:" << pid << "Limit:" << cpuPercent << "%";

    // 检查 cgroup 是否可用
    QDir cgroupDir("/sys/fs/cgroup/cpu");
    if (!cgroupDir.exists()) {
        QMessageBox::warning(this, "Cgroup Error",
                            "Cgroup not available on this system.\n"
                            "Mount cgroup: mount -t cgroup -o cpu cpu /sys/fs/cgroup/cpu");
        return;
    }

    QString cgroupName = QString("process_%1").arg(pid);
    QString cgroupPath = "/sys/fs/cgroup/cpu/" + cgroupName;

    // 创建 cgroup
    QProcess process;
    process.start("sudo", QStringList() << "mkdir" << "-p" << cgroupPath);
    process.waitForFinished();

    // 设置 CPU 限制
    int period = 100000;  // 100ms
    int quota = period * cpuPercent / 100;

    // 写入 cpu.cfs_quota_us
    process.start("sudo", QStringList() << "sh" << "-c"
                  << QString("echo %1 > %2/cpu.cfs_quota_us").arg(quota).arg(cgroupPath));
    process.waitForFinished();

    // 写入 cpu.cfs_period_us
    process.start("sudo", QStringList() << "sh" << "-c"
                  << QString("echo %1 > %2/cpu.cfs_period_us").arg(period).arg(cgroupPath));
    process.waitForFinished();

    // 将进程加入 cgroup
    process.start("sudo", QStringList() << "sh" << "-c"
                  << QString("echo %1 > %2/tasks").arg(pid).arg(cgroupPath));
    process.waitForFinished();

    QMessageBox::information(this, "Cgroup Setting",
                            QString("Process %1 CPU limited to %2%").arg(pid).arg(cpuPercent));
}

void MainWindow::removeProcessCgroupCpu(int pid)
{
    qDebug() << "Removing CPU cgroup for PID:" << pid;

    QString cgroupName = QString("process_%1").arg(pid);
    QString cgroupPath = "/sys/fs/cgroup/cpu/" + cgroupName;

    QProcess process;
    process.start("sudo", QStringList() << "rmdir" << cgroupPath);
    process.waitForFinished();

    QMessageBox::information(this, "Cgroup Setting", "CPU limit removed");
}

void MainWindow::setProcessCgroupMemory(int pid, int memMB)
{
    qDebug() << "Setting memory cgroup for PID:" << pid << "Limit:" << memMB << "MB";

    // 检查 cgroup 是否可用
    QDir cgroupDir("/sys/fs/cgroup/memory");
    if (!cgroupDir.exists()) {
        QMessageBox::warning(this, "Cgroup Error", "Memory cgroup not available");
        return;
    }

    QString cgroupName = QString("process_%1").arg(pid);
    QString cgroupPath = "/sys/fs/cgroup/memory/" + cgroupName;

    // 创建 cgroup
    QProcess process;
    process.start("sudo", QStringList() << "mkdir" << "-p" << cgroupPath);
    process.waitForFinished();

    // 设置内存限制（字节）
    long long limitBytes = static_cast<long long>(memMB) * 1024 * 1024;

    process.start("sudo", QStringList() << "sh" << "-c"
                  << QString("echo %1 > %2/memory.limit_in_bytes").arg(limitBytes).arg(cgroupPath));
    process.waitForFinished();

    // 将进程加入 cgroup
    process.start("sudo", QStringList() << "sh" << "-c"
                  << QString("echo %1 > %2/tasks").arg(pid).arg(cgroupPath));
    process.waitForFinished();

    QMessageBox::information(this, "Cgroup Setting",
                            QString("Process %1 memory limited to %2 MB").arg(pid).arg(memMB));
}

void MainWindow::removeProcessCgroupMemory(int pid)
{
    qDebug() << "Removing memory cgroup for PID:" << pid;

    QString cgroupName = QString("process_%1").arg(pid);
    QString cgroupPath = "/sys/fs/cgroup/memory/" + cgroupName;

    QProcess process;
    process.start("sudo", QStringList() << "rmdir" << cgroupPath);
    process.waitForFinished();

    QMessageBox::information(this, "Cgroup Setting", "Memory limit removed");
}
