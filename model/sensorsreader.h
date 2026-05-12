// model/sensorsreader.h
#ifndef SENSORSREADER_H
#define SENSORSREADER_H

#include <QObject>
#include <QTimer>
#include <QVector>
#include <QMutex>

struct TemperatureSensor {
    QString name;
    double value;
    QString unit;
    bool valid;
};

struct VoltageSensor {
    QString name;
    double value;
    QString unit;
    bool valid;
};

struct FanSensor {
    QString name;
    int rpm;
    bool valid;
};

class SensorsReader : public QObject
{
    Q_OBJECT
public:
    static SensorsReader& instance();

    void startMonitoring(int intervalMs = 1000);
    void stopMonitoring();

    // 获取传感器数据
    QVector<TemperatureSensor> getTemperatures() const;
    QVector<VoltageSensor> getVoltages() const;
    QVector<FanSensor> getFans() const;

signals:
    void sensorsUpdated();

private:
    SensorsReader();
    ~SensorsReader();

    void readSensors();
    void readFromLmSensors();
    void readFromHwmon();
    void parseLmSensorsOutput(const QString& output);

    QTimer* m_timer;
    bool m_running;
    mutable QMutex m_mutex;

    QVector<TemperatureSensor> m_temperatures;
    QVector<VoltageSensor> m_voltages;
    QVector<FanSensor> m_fans;
};

#endif
