// model/sensorsreader.cpp
#include "sensorsreader.h"
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>

SensorsReader::SensorsReader()
    : m_timer(nullptr)
    , m_running(false)
{
}

SensorsReader::~SensorsReader()
{
    stopMonitoring();
}

SensorsReader& SensorsReader::instance()
{
    static SensorsReader instance;
    return instance;
}

void SensorsReader::startMonitoring(int intervalMs)
{
    if (m_running) return;

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SensorsReader::readSensors);
    m_timer->start(intervalMs);
    m_running = true;

    // 立即读取一次
    readSensors();
}

void SensorsReader::stopMonitoring()
{
    if (m_timer) {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;
    }
    m_running = false;
}

void SensorsReader::readSensors()
{
    // 优先使用 lm-sensors
    readFromLmSensors();

    // 如果 lm-sensors 没有数据，尝试直接读取 hwmon
    if (m_temperatures.isEmpty() && m_fans.isEmpty() && m_voltages.isEmpty()) {
        readFromHwmon();
    }

    emit sensorsUpdated();
}

void SensorsReader::readFromLmSensors()
{
    QProcess process;
    process.start("sensors", QStringList(), QIODevice::ReadOnly);

    if (process.waitForFinished(2000)) {
        QString output = QString::fromUtf8(process.readAllStandardOutput());
        if (!output.isEmpty()) {
            parseLmSensorsOutput(output);
            return;
        }
    }

    // sensors 命令失败，清空数据
    QMutexLocker locker(&m_mutex);
    m_temperatures.clear();
    m_voltages.clear();
    m_fans.clear();
}

void SensorsReader::parseLmSensorsOutput(const QString& output)
{
    QMutexLocker locker(&m_mutex);

    m_temperatures.clear();
    m_voltages.clear();
    m_fans.clear();

    QStringList lines = output.split('\n');
    QString currentChip;

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();

        // 检测芯片名称（不以空格开头的行）
        if (!trimmed.isEmpty() && !trimmed.startsWith(" ") && !trimmed.startsWith("\t")) {
            currentChip = trimmed;
            continue;
        }

        // 温度检测
        if (trimmed.contains("°C") && trimmed.contains("+")) {
            QRegularExpression tempRe("\\+([0-9.]+)°C");
            QRegularExpressionMatch match = tempRe.match(trimmed);
            if (match.hasMatch()) {
                TemperatureSensor temp;
                temp.value = match.captured(1).toDouble();
                temp.unit = "°C";
                temp.valid = true;

                // 提取传感器名称
                QString name = trimmed.split(":").value(0).trimmed();
                if (name.isEmpty()) name = currentChip;
                temp.name = QString("%1 - %2").arg(currentChip).arg(name);

                m_temperatures.append(temp);
            }
        }

        // 风扇转速检测
        if (trimmed.contains("RPM") && trimmed.contains("+")) {
            QRegularExpression fanRe("\\+([0-9]+) RPM");
            QRegularExpressionMatch match = fanRe.match(trimmed);
            if (match.hasMatch()) {
                FanSensor fan;
                fan.rpm = match.captured(1).toInt();
                fan.valid = true;

                QString name = trimmed.split(":").value(0).trimmed();
                if (name.isEmpty()) name = currentChip;
                fan.name = QString("%1 - %2").arg(currentChip).arg(name);

                m_fans.append(fan);
            }
        }

        // 电压检测
        if (trimmed.contains("V") && trimmed.contains("+") && !trimmed.contains("°C")) {
            QRegularExpression voltRe("\\+([0-9.]+) V");
            QRegularExpressionMatch match = voltRe.match(trimmed);
            if (match.hasMatch()) {
                VoltageSensor volt;
                volt.value = match.captured(1).toDouble();
                volt.unit = "V";
                volt.valid = true;

                QString name = trimmed.split(":").value(0).trimmed();
                if (name.isEmpty()) name = currentChip;
                volt.name = QString("%1 - %2").arg(currentChip).arg(name);

                m_voltages.append(volt);
            }
        }
    }
}

void SensorsReader::readFromHwmon()
{
    QMutexLocker locker(&m_mutex);

    m_temperatures.clear();
    m_voltages.clear();
    m_fans.clear();

    QDir hwmonDir("/sys/class/hwmon");

    for (const QString& hwmon : hwmonDir.entryList()) {
        QDir deviceDir("/sys/class/hwmon/" + hwmon);

        // 获取设备名称
        QString deviceName = hwmon;
        QFile nameFile(deviceDir.absoluteFilePath("name"));
        if (nameFile.open(QIODevice::ReadOnly)) {
            deviceName = QString::fromUtf8(nameFile.readAll()).trimmed();
            nameFile.close();
        }

        // 读取温度
        for (const QString& file : deviceDir.entryList()) {
            if (file.startsWith("temp") && file.endsWith("_input")) {
                QFile tempFile(deviceDir.absoluteFilePath(file));
                if (tempFile.open(QIODevice::ReadOnly)) {
                    double val = tempFile.readAll().trimmed().toDouble() / 1000.0;
                    tempFile.close();

                    if (val > 0 && val < 120) {
                        TemperatureSensor temp;
                        temp.value = val;
                        temp.unit = "°C";
                        temp.valid = true;

                        // 获取标签
                        QString label = file;
                        QString labelFile = file;
                        labelFile.replace("_input", "_label");
                        QFile labelF(deviceDir.absoluteFilePath(labelFile));
                        if (labelF.open(QIODevice::ReadOnly)) {
                            label = QString::fromUtf8(labelF.readAll()).trimmed();
                            labelF.close();
                        }

                        temp.name = QString("%1 - %2").arg(deviceName).arg(label);
                        m_temperatures.append(temp);
                    }
                }
            }

            // 读取风扇
            if (file.startsWith("fan") && file.endsWith("_input")) {
                QFile fanFile(deviceDir.absoluteFilePath(file));
                if (fanFile.open(QIODevice::ReadOnly)) {
                    int rpm = fanFile.readAll().trimmed().toInt();
                    fanFile.close();

                    if (rpm > 0) {
                        FanSensor fan;
                        fan.rpm = rpm;
                        fan.valid = true;

                        QString label = file;
                        QString labelFile = file;
                        labelFile.replace("_input", "_label");
                        QFile labelF(deviceDir.absoluteFilePath(labelFile));
                        if (labelF.open(QIODevice::ReadOnly)) {
                            label = QString::fromUtf8(labelF.readAll()).trimmed();
                            labelF.close();
                        }

                        fan.name = QString("%1 - %2").arg(deviceName).arg(label);
                        m_fans.append(fan);
                    }
                }
            }

            // 读取电压
            if (file.startsWith("in") && file.endsWith("_input")) {
                QFile voltFile(deviceDir.absoluteFilePath(file));
                if (voltFile.open(QIODevice::ReadOnly)) {
                    double val = voltFile.readAll().trimmed().toDouble();
                    voltFile.close();

                    // 转换为伏特
                    if (val > 1000) val = val / 1000.0;
                    val = val / 1000.0;

                    if (val > 0 && val < 20) {
                        VoltageSensor volt;
                        volt.value = val;
                        volt.unit = "V";
                        volt.valid = true;

                        QString label = file;
                        QString labelFile = file;
                        labelFile.replace("_input", "_label");
                        QFile labelF(deviceDir.absoluteFilePath(labelFile));
                        if (labelF.open(QIODevice::ReadOnly)) {
                            label = QString::fromUtf8(labelF.readAll()).trimmed();
                            labelF.close();
                        }

                        volt.name = QString("%1 - %2").arg(deviceName).arg(label);
                        m_voltages.append(volt);
                    }
                }
            }
        }
    }
}

QVector<TemperatureSensor> SensorsReader::getTemperatures() const
{
    QMutexLocker locker(&m_mutex);
    return m_temperatures;
}

QVector<VoltageSensor> SensorsReader::getVoltages() const
{
    QMutexLocker locker(&m_mutex);
    return m_voltages;
}

QVector<FanSensor> SensorsReader::getFans() const
{
    QMutexLocker locker(&m_mutex);
    return m_fans;
}
