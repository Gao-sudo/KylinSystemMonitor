#include "exportcontroller.h"
#include "../service/cryptohelper.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDebug>

ExportController& ExportController::instance()
{
    static ExportController instance;
    return instance;
}

QString ExportController::formatCsvValue(const QString& value)
{
    QString result = value;
    if (result.contains(',') || result.contains('"') || result.contains('\n')) {
        result.replace('"', "\"\"");
        result = "\"" + result + "\"";
    }
    return result;
}

bool ExportController::exportToCsv(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Failed to open file:" << filePath;
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out.setGenerateByteOrderMark(true);

    // 表头
    out << "Time,CPU(%),Memory Total(MB),Memory Available(MB),Memory Used(%),"
        << "Disk Read(MB/s),Disk Write(MB/s),Network RX(MB/s),Network TX(MB/s)\n";

    SystemModel* model = SystemModel::instance();
    auto cpuHistory = model->getCpuHistory(100);
    auto memHistory = model->getMemHistory(100);
    auto diskReadHistory = model->getDiskReadHistory(100);
    auto diskWriteHistory = model->getDiskWriteHistory(100);
    auto netRxHistory = model->getNetRxHistory(100);
    auto netTxHistory = model->getNetTxHistory(100);

    for (int i = 0; i < cpuHistory.size(); ++i) {
        double memTotalMB = model->memTotal() / 1024.0;
        double memAvailMB = memHistory[i];
        double memUsedPercent = (memTotalMB > 0) ? 100.0 * (memTotalMB - memAvailMB) / memTotalMB : 0;

        out << QDateTime::currentDateTime().addSecs(-cpuHistory.size() + i).toString("yyyy-MM-dd HH:mm:ss") << ","
            << QString::number(cpuHistory[i], 'f', 1) << ","
            << QString::number(memTotalMB, 'f', 0) << ","
            << QString::number(memAvailMB, 'f', 0) << ","
            << QString::number(memUsedPercent, 'f', 1) << ","
            << QString::number(diskReadHistory.value(i, 0), 'f', 2) << ","
            << QString::number(diskWriteHistory.value(i, 0), 'f', 2) << ","
            << QString::number(netRxHistory.value(i, 0), 'f', 2) << ","
            << QString::number(netTxHistory.value(i, 0), 'f', 2) << "\n";
    }

    file.close();
    return true;
}

bool ExportController::exportToJson(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Failed to open file:" << filePath;
        return false;
    }

    QJsonObject root;
    root["export_time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    root["app_name"] = "Kylin System Monitor";

    SystemModel* model = SystemModel::instance();

    QJsonObject current;
    current["cpu_usage"] = model->cpuUsage();
    current["mem_total_mb"] = model->memTotal() / 1024.0;
    current["mem_available_mb"] = model->memAvail() / 1024.0;
    current["disk_read_mb"] = model->diskRead();
    current["disk_write_mb"] = model->diskWrite();
    current["net_rx_mb"] = model->netRx();
    current["net_tx_mb"] = model->netTx();
    root["current"] = current;

    QJsonArray history;
    auto cpuHistory = model->getCpuHistory(100);
    auto memHistory = model->getMemHistory(100);
    for (int i = 0; i < cpuHistory.size(); ++i) {
        QJsonObject point;
        point["time"] = QDateTime::currentDateTime().addSecs(-cpuHistory.size() + i).toString("yyyy-MM-dd HH:mm:ss");
        point["cpu"] = cpuHistory[i];
        point["mem_mb"] = memHistory[i];
        history.append(point);
    }
    root["history"] = history;

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

bool ExportController::exportEncryptedToJson(const QString& filePath)
{
    QJsonObject root;
    root["export_time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    root["app_name"] = "Kylin System Monitor";

    SystemModel* model = SystemModel::instance();

    QJsonObject current;
    current["cpu_usage"] = model->cpuUsage();
    current["mem_total_mb"] = model->memTotal() / 1024.0;
    current["mem_available_mb"] = model->memAvail() / 1024.0;
    current["disk_read_mb"] = model->diskRead();
    current["disk_write_mb"] = model->diskWrite();
    current["net_rx_mb"] = model->netRx();
    current["net_tx_mb"] = model->netTx();
    root["current"] = current;

    QJsonArray history;
    auto cpuHistory = model->getCpuHistory(100);
    auto memHistory = model->getMemHistory(100);
    for (int i = 0; i < cpuHistory.size(); ++i) {
        QJsonObject point;
        point["time"] = QDateTime::currentDateTime().addSecs(-cpuHistory.size() + i).toString("yyyy-MM-dd HH:mm:ss");
        point["cpu"] = cpuHistory[i];
        point["mem_mb"] = memHistory[i];
        history.append(point);
    }
    root["history"] = history;

    return CryptoHelper::instance().exportEncryptedData(filePath, root);
}

bool ExportController::exportProcessToCsv(int pid, const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out.setGenerateByteOrderMark(true);

    out << "PID,Name,CPU(%),Memory(MB),State,Threads,Nice,FDs,PPID,IPC\n";

    ProcessModel* model = ProcessModel::instance();
    auto info = model->getProcessById(pid);
    if (info.pid != -1) {
        out << info.pid << ","
            << formatCsvValue(info.name) << ","
            << QString::number(info.cpu, 'f', 1) << ","
            << QString::number(info.mem, 'f', 1) << ","
            << info.state << ","
            << info.threads << ","
            << info.nice << ","
            << info.fdCount << ","
            << info.ppid << ","
            << info.ipcInfo << "\n";
    }

    file.close();
    return true;
}

bool ExportController::exportProcessToJson(int pid, const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonObject root;
    root["export_time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    root["pid"] = pid;

    ProcessModel* model = ProcessModel::instance();
    auto info = model->getProcessById(pid);
    if (info.pid != -1) {
        root["name"] = info.name;
        root["cmdline"] = info.cmdline;
        root["cpu"] = info.cpu;
        root["mem_mb"] = info.mem;
        root["state"] = info.state;
        root["threads"] = info.threads;
        root["nice"] = info.nice;
        root["fd_count"] = info.fdCount;
        root["ppid"] = info.ppid;
        root["ipc"] = info.ipcInfo;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    return true;
}
