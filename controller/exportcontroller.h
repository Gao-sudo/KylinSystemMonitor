#ifndef EXPORTCONTROLLER_H
#define EXPORTCONTROLLER_H

#include <QObject>
#include <QString>
#include "../model/systemmodel.h"
#include "../model/processmodel.h"

class ExportController : public QObject
{
    Q_OBJECT
public:
    static ExportController& instance();

    // 系统数据导出
    bool exportToCsv(const QString& filePath);
    bool exportToJson(const QString& filePath);
    bool exportEncryptedToJson(const QString& filePath);

    // 进程数据导出
    bool exportProcessToCsv(int pid, const QString& filePath);
    bool exportProcessToJson(int pid, const QString& filePath);

private:
    ExportController() = default;
    QString formatCsvValue(const QString& value);
};

#endif
