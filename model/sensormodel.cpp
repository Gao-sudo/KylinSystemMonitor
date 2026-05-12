#include "sensormodel.h"
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QMutexLocker>

SensorModel::SensorModel()
    : m_timer(nullptr)
{
}

SensorModel::~SensorModel()
{
    stopMonitoring();
}

SensorModel* SensorModel::instance()
{
    static SensorModel instance;
    return &instance;
}

void SensorModel::startMonitoring(int intervalMs)
{
    if (m_timer) return;

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SensorModel::readSensors);
    m_timer->start(intervalMs);
    readSensors();
}

void SensorModel::stopMonitoring()
{
    if (m_timer) {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;
    }
}

void SensorModel::readSensors()
{
    readFromLmSensors();
    if (m_temperatures.isEmpty() && m_fans.isEmpty() && m_voltages.isEmpty()) {
        readFromHwmon();
    }
    emit sensorsUpdated();
}

void SensorModel::readFromLmSensors()
{
    QMutexLocker locker(&m_mutex);
    m_temperatures.clear();
    m_voltages.clear();
    m_fans.clear();

    QProcess process;
    process.start("sensors", QStringList());
    if (!process.waitForFinished(2000)) return;

    QString output = QString::fromUtf8(process.readAllStandardOutput());
    if (output.isEmpty()) return;

    QStringList lines = output.split('\n');
    QString currentChip;

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();

        if (!trimmed.isEmpty() && !trimmed.startsWith(" ") && !trimmed.startsWith("\t")) {
            currentChip = trimmed;
            continue;
        }

        // 温度检测
        if (trimmed.contains("°C") && trimmed.contains("+")) {
            QRegularExpression tempRe("\\+([0-9.]+)°C");
            auto match = tempRe.match(trimmed);
            if (match.hasMatch()) {
                TemperatureSensor temp;
                temp.value = match.captured(1).toDouble();
                temp.unit = "°C";
                temp.valid = true;
                temp.name = trimmed.split(":").value(0).trimmed();
                if (temp.name.isEmpty()) temp.name = currentChip;
                m_temperatures.append(temp);
            }
        }

        // 风扇检测
        if (trimmed.contains("RPM") && trimmed.contains("+")) {
            QRegularExpression fanRe("\\+([0-9]+) RPM");
            auto match = fanRe.match(trimmed);
            if (match.hasMatch()) {
                FanSensor fan;
                fan.rpm = match.captured(1).toInt();
                fan.valid = true;
                fan.name = trimmed.split(":").value(0).trimmed();
                if (fan.name.isEmpty()) fan.name = currentChip;
                m_fans.append(fan);
            }
        }

        // 电压检测
        if (trimmed.contains("V") && trimmed.contains("+") && !trimmed.contains("°C")) {
            QRegularExpression voltRe("\\+([0-9.]+) V");
            auto match = voltRe.match(trimmed);
            if (match.hasMatch()) {
                VoltageSensor volt;
                volt.value = match.captured(1).toDouble();
                volt.unit = "V";
                volt.valid = true;
                volt.name = trimmed.split(":").value(0).trimmed();
                if (volt.name.isEmpty()) volt.name = currentChip;
                m_voltages.append(volt);
            }
        }
    }
}

void SensorModel::readFromHwmon()
{
    QMutexLocker locker(&m_mutex);
    m_temperatures.clear();
    m_fans.clear();
    m_voltages.clear();

    QDir hwmonDir("/sys/class/hwmon");
    for (const QString& hwmon : hwmonDir.entryList()) {
        QDir deviceDir("/sys/class/hwmon/" + hwmon);

        QString deviceName = hwmon;
        QFile nameFile(deviceDir.absoluteFilePath("name"));
        if (nameFile.open(QIODevice::ReadOnly)) {
            deviceName = QString::fromUtf8(nameFile.readAll()).trimmed();
            nameFile.close();
        }

        // 温度
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
                        temp.name = QString("%1 - %2").arg(deviceName).arg(file);
                        m_temperatures.append(temp);
                    }
                }
            }

            // 风扇
            if (file.startsWith("fan") && file.endsWith("_input")) {
                QFile fanFile(deviceDir.absoluteFilePath(file));
                if (fanFile.open(QIODevice::ReadOnly)) {
                    int rpm = fanFile.readAll().trimmed().toInt();
                    fanFile.close();
                    if (rpm > 0) {
                        FanSensor fan;
                        fan.rpm = rpm;
                        fan.valid = true;
                        fan.name = QString("%1 - %2").arg(deviceName).arg(file);
                        m_fans.append(fan);
                    }
                }
            }

            // 电压
            if (file.startsWith("in") && file.endsWith("_input")) {
                QFile voltFile(deviceDir.absoluteFilePath(file));
                if (voltFile.open(QIODevice::ReadOnly)) {
                    double val = voltFile.readAll().trimmed().toDouble();
                    voltFile.close();
                    if (val > 1000) val /= 1000.0;
                    val /= 1000.0;
                    if (val > 0 && val < 20) {
                        VoltageSensor volt;
                        volt.value = val;
                        volt.unit = "V";
                        volt.valid = true;
                        volt.name = QString("%1 - %2").arg(deviceName).arg(file);
                        m_voltages.append(volt);
                    }
                }
            }
        }
    }
}

QVector<TemperatureSensor> SensorModel::getTemperatures() const
{
    QMutexLocker locker(&m_mutex);
    return m_temperatures;
}

QVector<VoltageSensor> SensorModel::getVoltages() const
{
    QMutexLocker locker(&m_mutex);
    return m_voltages;
}

QVector<FanSensor> SensorModel::getFans() const
{
    QMutexLocker locker(&m_mutex);
    return m_fans;
}
