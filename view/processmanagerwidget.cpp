// view/processmanagerwidget.cpp
#include "processmanagerwidget.h"
#include "../model/processmodel.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>
#include <QProcess>
#include <QDebug>
#include <QTreeWidgetItem>
#include <QDateTime>
#include <QFileDialog>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

ProcessManagerWidget::ProcessManagerWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}

void ProcessManagerWidget::initUI()
{
    QHBoxLayout *layProcess = new QHBoxLayout(this);

    m_tableProcess = new QTableWidget;
    m_tableProcess->setColumnCount(11);
    m_tableProcess->setHorizontalHeaderLabels({
        "PID", "Name", "CPU%", "Memory(MB)", "Threads", "State", "PPID", "Nice", "FDs", "IPC", "Alert"
    });
    m_tableProcess->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tableProcess->horizontalHeader()->setStretchLastSection(true);

    m_treeProcess = new QTreeWidget;
    m_treeProcess->setHeaderLabel("Process Tree");

    layProcess->addWidget(m_tableProcess, 2);
    layProcess->addWidget(m_treeProcess, 1);

    connect(m_tableProcess, &QTableWidget::customContextMenuRequested,
            this, &ProcessManagerWidget::onTableRightClick);
}

void ProcessManagerWidget::onTableRightClick(const QPoint &pos)
{
    showContextMenu(pos);
}

void ProcessManagerWidget::showContextMenu(const QPoint &pos)
{
    QTableWidgetItem *item = m_tableProcess->itemAt(pos);
    if (!item) return;

    int row = m_tableProcess->currentRow();
    if (row < 0) return;

    bool ok;
    int pid = m_tableProcess->item(row, 0)->text().toInt(&ok);
    if (!ok || pid <= 0) return;

    QString processName = m_tableProcess->item(row, 1)->text();
    int currentNice = m_tableProcess->item(row, 7)->text().toInt();

    QMenu menu;

    // ========== 1. Process Control ==========
    QMenu *controlMenu = menu.addMenu("Process Control");
    controlMenu->addAction("Kill Process", [=]() {
        emit killProcessRequested(pid);
        qDebug() << "Kill requested for PID:" << pid;
    });
    controlMenu->addAction("Force Kill (SIGKILL)", [=]() {
        emit forceKillProcessRequested(pid);
        qDebug() << "Force kill requested for PID:" << pid;
    });
    controlMenu->addAction("Suspend Process", [=]() {
        emit suspendProcessRequested(pid);
        qDebug() << "Suspend requested for PID:" << pid;
    });
    controlMenu->addAction("Resume Process", [=]() {
        emit resumeProcessRequested(pid);
        qDebug() << "Resume requested for PID:" << pid;
    });

    menu.addSeparator();

    // ========== 2. Adjust Priority ==========
    QMenu *priorityMenu = menu.addMenu("Adjust Priority");

    // 2.1 Nice 值调整
    QMenu *niceMenu = priorityMenu->addMenu("Nice Value");
    niceMenu->addAction("Highest Priority (-20)", [=]() {
        emit changePriorityRequested(pid, -20);
        qDebug() << "Nice change requested for PID:" << pid << "to -20";
    });
    niceMenu->addAction("High Priority (-10)", [=]() {
        emit changePriorityRequested(pid, -10);
        qDebug() << "Nice change requested for PID:" << pid << "to -10";
    });
    niceMenu->addAction("Normal Priority (0)", [=]() {
        emit changePriorityRequested(pid, 0);
        qDebug() << "Nice change requested for PID:" << pid << "to 0";
    });
    niceMenu->addAction("Low Priority (10)", [=]() {
        emit changePriorityRequested(pid, 10);
        qDebug() << "Nice change requested for PID:" << pid << "to 10";
    });
    niceMenu->addAction("Lowest Priority (19)", [=]() {
        emit changePriorityRequested(pid, 19);
        qDebug() << "Nice change requested for PID:" << pid << "to 19";
    });
    niceMenu->addSeparator();
    niceMenu->addAction("Custom...", [=]() {
        bool ok;
        int nice = QInputDialog::getInt(this, "Set Nice Value",
                                       "Nice value range: -20 to 19\n(-20=Highest, 19=Lowest)",
                                       currentNice, -20, 19, 1, &ok);
        if (ok) {
            emit changePriorityRequested(pid, nice);
            qDebug() << "Custom nice change requested for PID:" << pid << "to" << nice;
        }
    });

    // 2.2 Cgroup 控制
    QMenu *cgroupMenu = priorityMenu->addMenu("Cgroup Control");
    cgroupMenu->addAction("Limit CPU to 10%", [=]() {
        emit cgroupCpuRequested(pid, 10);
        qDebug() << "Cgroup CPU 10% requested for PID:" << pid;
    });
    cgroupMenu->addAction("Limit CPU to 25%", [=]() {
        emit cgroupCpuRequested(pid, 25);
        qDebug() << "Cgroup CPU 25% requested for PID:" << pid;
    });
    cgroupMenu->addAction("Limit CPU to 50%", [=]() {
        emit cgroupCpuRequested(pid, 50);
        qDebug() << "Cgroup CPU 50% requested for PID:" << pid;
    });
    cgroupMenu->addAction("Limit CPU to 75%", [=]() {
        emit cgroupCpuRequested(pid, 75);
        qDebug() << "Cgroup CPU 75% requested for PID:" << pid;
    });
    cgroupMenu->addAction("Remove CPU Limit", [=]() {
        emit cgroupCpuRemoveRequested(pid);
        qDebug() << "Cgroup CPU remove requested for PID:" << pid;
    });
    cgroupMenu->addSeparator();
    cgroupMenu->addAction("Limit Memory to 512MB", [=]() {
        emit cgroupMemoryRequested(pid, 512);
        qDebug() << "Cgroup memory 512MB requested for PID:" << pid;
    });
    cgroupMenu->addAction("Limit Memory to 1GB", [=]() {
        emit cgroupMemoryRequested(pid, 1024);
        qDebug() << "Cgroup memory 1GB requested for PID:" << pid;
    });
    cgroupMenu->addAction("Limit Memory to 2GB", [=]() {
        emit cgroupMemoryRequested(pid, 2048);
        qDebug() << "Cgroup memory 2GB requested for PID:" << pid;
    });
    cgroupMenu->addAction("Remove Memory Limit", [=]() {
        emit cgroupMemoryRemoveRequested(pid);
        qDebug() << "Cgroup memory remove requested for PID:" << pid;
    });

    menu.addSeparator();

    // ========== 3. Resource Report ==========
    QMenu *reportMenu = menu.addMenu("Resource Report");
    reportMenu->addAction("Generate Real-time Report", [=]() {
        emit generateReportRequested(pid, processName);
        qDebug() << "Generate report requested for PID:" << pid;
    });
    reportMenu->addAction("Export to CSV", [=]() {
        emit exportCsvRequested(pid, processName);
        qDebug() << "Export CSV requested for PID:" << pid;
    });
    reportMenu->addAction("Export to JSON", [=]() {
        emit exportJsonRequested(pid, processName);
        qDebug() << "Export JSON requested for PID:" << pid;
    });

    menu.exec(m_tableProcess->viewport()->mapToGlobal(pos));
}

void ProcessManagerWidget::updateProcessList(const QList<ProcessInfo>& processes)
{
    m_tableProcess->setRowCount(0);

    for (const auto& p : processes) {
        int row = m_tableProcess->rowCount();
        m_tableProcess->insertRow(row);

        m_tableProcess->setItem(row, 0, new QTableWidgetItem(QString::number(p.pid)));
        m_tableProcess->setItem(row, 1, new QTableWidgetItem(p.name));
        m_tableProcess->setItem(row, 2, new QTableWidgetItem(QString::number(p.cpu, 'f', 1)));
        m_tableProcess->setItem(row, 3, new QTableWidgetItem(QString::number(p.mem, 'f', 1)));
        m_tableProcess->setItem(row, 4, new QTableWidgetItem(QString::number(p.threads)));
        m_tableProcess->setItem(row, 5, new QTableWidgetItem(p.state));
        m_tableProcess->setItem(row, 6, new QTableWidgetItem(QString::number(p.ppid)));

        QTableWidgetItem *niceItem = new QTableWidgetItem(QString::number(p.nice));
        if (p.nice < 0) niceItem->setForeground(Qt::green);
        else if (p.nice > 0) niceItem->setForeground(Qt::red);
        m_tableProcess->setItem(row, 7, niceItem);

        m_tableProcess->setItem(row, 8, new QTableWidgetItem(QString::number(p.fdCount)));

        QString ipcDisplay = p.ipcInfo.isEmpty() ? "-" : p.ipcInfo;
        QTableWidgetItem *ipcItem = new QTableWidgetItem(ipcDisplay);
        if (ipcDisplay.contains("socket")) ipcItem->setForeground(QColor(52, 152, 219));
        else if (ipcDisplay.contains("pipe")) ipcItem->setForeground(QColor(46, 204, 113));
        m_tableProcess->setItem(row, 9, ipcItem);

        QString alarm = "";
        QColor textColor = Qt::black;
        if (p.cpu > 80.0) { alarm = "High CPU"; textColor = Qt::red; }
        if (p.state == "Z") { alarm = alarm.isEmpty() ? "Zombie" : alarm + " / Zombie"; textColor = Qt::red; }
        QTableWidgetItem *alarmItem = new QTableWidgetItem(alarm);
        alarmItem->setForeground(textColor);
        m_tableProcess->setItem(row, 10, alarmItem);
    }

    m_tableProcess->resizeColumnsToContents();
}

void ProcessManagerWidget::updateProcessTree(const QVector<ProcessTreeNode>& tree)
{
    m_treeProcess->clear();

    if (tree.isEmpty()) return;

    QMap<int, const ProcessTreeNode*> pidToNode;
    for (const auto& node : tree) {
        pidToNode[node.pid] = &node;
    }

    std::function<void(const ProcessTreeNode&, QTreeWidgetItem*)> addNode;
    addNode = [this, &addNode, &pidToNode](const ProcessTreeNode& node, QTreeWidgetItem* parentItem) {
        QString displayText = QString("PID: %1 | %2").arg(node.pid).arg(node.name);
        if (!node.cmdline.isEmpty() && node.cmdline != node.name) {
            QString shortCmd = node.cmdline.left(50);
            if (shortCmd.length() < node.cmdline.length()) shortCmd += "...";
            displayText = QString("PID: %1 | %2 [%3]").arg(node.pid).arg(node.name).arg(shortCmd);
        }

        QTreeWidgetItem *item;
        if (parentItem) {
            item = new QTreeWidgetItem(parentItem);
        } else {
            item = new QTreeWidgetItem(m_treeProcess);
        }
        item->setText(0, displayText);

        // 修复：使用指针
        ProcessModel* model = ProcessModel::instance();
        ProcessInfo info = model->getProcessById(node.pid);
        if (info.state == "Z") {
            item->setForeground(0, Qt::red);
        } else if (info.cpu > 80.0) {
            item->setForeground(0, QColor(255, 100, 0));
        }

        for (int childPid : node.children) {
            if (pidToNode.contains(childPid)) {
                addNode(*pidToNode[childPid], item);
            }
        }
    };

    for (const auto& node : tree) {
        if (node.ppid == 0 || node.ppid == 1 || node.ppid == node.pid) {
            addNode(node, nullptr);
        }
    }

    m_treeProcess->expandAll();
}
