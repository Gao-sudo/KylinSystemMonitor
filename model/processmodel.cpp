#include "processmodel.h"
#include <QProcess>
#include <QDebug>
#include <algorithm>
#include <QMap>

ProcessModel* ProcessModel::instance()
{
    static ProcessModel instance;
    return &instance;
}
QList<ProcessInfo> ProcessModel::getProcessList() const
{
    QMutexLocker locker(&m_mutex);
    return m_processList;
}

ProcessInfo ProcessModel::getProcessById(int pid) const
{
    QMutexLocker locker(&m_mutex);
    for (const auto& proc : m_processList) {
        if (proc.pid == pid) {
            return proc;
        }
    }
    ProcessInfo empty;
    empty.pid = -1;
    return empty;
}

QVector<ProcessTreeNode> ProcessModel::getProcessTree() const
{
    QMutexLocker locker(&m_mutex);
    return m_processTree;
}

void ProcessModel::updateProcessList(const QList<ProcessInfo>& list)
{
    {
        QMutexLocker locker(&m_mutex);
        m_processList = list;
    }
    rebuildProcessTree();
    emit processListChanged();
}

void ProcessModel::updateProcessInfo(int pid, const ProcessInfo& info)
{
    QMutexLocker locker(&m_mutex);
    for (int i = 0; i < m_processList.size(); ++i) {
        if (m_processList[i].pid == pid) {
            m_processList[i] = info;
            break;
        }
    }
    rebuildProcessTree();
}

void ProcessModel::rebuildProcessTree()
{
    QMutexLocker locker(&m_mutex);

    m_processTree.clear();
    m_pidToTreeIndex.clear();

    if (m_processList.isEmpty()) return;

    // 创建所有节点的映射
    QMap<int, ProcessTreeNode> nodeMap;
    for (const auto& proc : m_processList) {
        ProcessTreeNode node;
        node.pid = proc.pid;
        node.ppid = proc.ppid;
        node.name = proc.name;
        node.cmdline = proc.cmdline;
        node.children.clear();
        nodeMap[proc.pid] = node;
    }

    // 建立父子关系
    for (auto& node : nodeMap) {
        if (node.ppid != node.pid && nodeMap.contains(node.ppid)) {
            nodeMap[node.ppid].children.append(node.pid);
        }
    }

    // 收集根节点 (ppid为0或1或不存在于映射中)
    for (auto& node : nodeMap) {
        if (node.ppid == 0 || node.ppid == 1 || node.ppid == node.pid || !nodeMap.contains(node.ppid)) {
            m_processTree.append(node);
            m_pidToTreeIndex[node.pid] = m_processTree.size() - 1;
        }
    }

    // 按PID排序根节点
    std::sort(m_processTree.begin(), m_processTree.end(),
              [](const ProcessTreeNode& a, const ProcessTreeNode& b) {
                  return a.pid < b.pid;
              });
}

bool ProcessModel::killProcess(int pid, int signal)
{
    QProcess process;
    process.start("kill", QStringList() << QString("-%1").arg(signal) << QString::number(pid));
    if (process.waitForFinished(2000)) {
        bool success = (process.exitCode() == 0);
        if (success) {
            qDebug() << "Process" << pid << "killed with signal" << signal;
        }
        return success;
    }
    return false;
}

// processmodel.cpp
bool ProcessModel::setNice(int pid, int niceValue)
{
    qDebug() << "setNice called for PID:" << pid << "Nice value:" << niceValue;

    QProcess process;
    process.start("renice", QStringList() << QString::number(niceValue) << "-p" << QString::number(pid));

    if (process.waitForFinished(2000)) {
        QString stderr = process.readAllStandardError();
        int exitCode = process.exitCode();

        qDebug() << "renice exit code:" << exitCode;
        qDebug() << "renice stderr:" << stderr;

        if (exitCode == 0) {
            qDebug() << "Nice value changed successfully";
            return true;
        } else {
            qDebug() << "Failed to change nice value";

            // 尝试使用 sudo
            QProcess sudoProcess;
            sudoProcess.start("sudo", QStringList() << "renice" << QString::number(niceValue)
                              << "-p" << QString::number(pid));
            sudoProcess.waitForFinished(2000);
            if (sudoProcess.exitCode() == 0) {
                qDebug() << "Nice value changed with sudo";
                return true;
            }
            return false;
        }
    }
    return false;
}
