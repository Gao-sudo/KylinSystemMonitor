#ifndef PROCESSMODEL_H
#define PROCESSMODEL_H

#include <QObject>
#include <QList>
#include <QVector>
#include <QMutex>
#include <QString>
#include <QMap>          // 添加这行

// 进程信息结构体
struct ProcessInfo {
    int pid;           // 进程ID
    int ppid;          // 父进程ID
    QString name;      // 进程名称
    QString cmdline;   // 完整命令行
    double cpu;        // CPU使用率 (%)
    double mem;        // 内存使用 (MB)
    QString state;     // 进程状态 (R/S/D/Z/T)
    int threads;       // 线程数
    int nice;          // Nice值 (-20到19)
    int fdCount;       // 打开的文件句柄数
    QString ipcInfo;   // IPC信息 (socket/pipe)
    QString user;      // 所属用户

    ProcessInfo()
        : pid(0)
        , ppid(0)
        , cpu(0.0)
        , mem(0.0)
        , threads(0)
        , nice(0)
        , fdCount(0)
    {}
};

// 进程树节点结构体
struct ProcessTreeNode {
    int pid;                    // 进程ID
    int ppid;                   // 父进程ID
    QString name;               // 进程名称
    QString cmdline;            // 命令行
    QVector<int> children;      // 子进程PID列表

    ProcessTreeNode()
        : pid(0)
        , ppid(0)
    {}
};

class ProcessModel : public QObject
{
    Q_OBJECT

public:
    // 单例模式
    static ProcessModel* instance();

    // 获取进程列表
    QList<ProcessInfo> getProcessList() const;

    // 获取单个进程信息
    ProcessInfo getProcessById(int pid) const;

    // 获取进程树
    QVector<ProcessTreeNode> getProcessTree() const;

    // 更新进程列表（由Controller调用）
    void updateProcessList(const QList<ProcessInfo>& list);

    // 更新单个进程信息
    void updateProcessInfo(int pid, const ProcessInfo& info);

    // 进程操作
    bool killProcess(int pid, int signal = 15);
    bool setNice(int pid, int niceValue);

signals:
    void processListChanged();
    void processAlert(int pid, const QString& name, const QString& message);

private:
    ProcessModel() = default;
    ~ProcessModel() = default;
    ProcessModel(const ProcessModel&) = delete;
    ProcessModel& operator=(const ProcessModel&) = delete;

    // 重建进程树
    void rebuildProcessTree();

    mutable QMutex m_mutex;
    QList<ProcessInfo> m_processList;
    QVector<ProcessTreeNode> m_processTree;
    QMap<int, int> m_pidToTreeIndex;  // 现在可以正确编译
};

#endif // PROCESSMODEL_H
