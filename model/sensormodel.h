#ifndef SENSORMODEL_H
#define SENSORMODEL_H

#include <QObject>
#include <QVector>
#include <QTimer>
#include <QMutex>
#include <QString>

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

class SensorModel : public QObject
{
    Q_OBJECT
public:
    static SensorModel* instance();  // 改为返回指针

    void startMonitoring(int intervalMs = 2000);
    void stopMonitoring();

    QVector<TemperatureSensor> getTemperatures() const;
    QVector<VoltageSensor> getVoltages() const;
    QVector<FanSensor> getFans() const;

signals:
    void sensorsUpdated();

private:
    SensorModel();
    ~SensorModel();

    void readSensors();
    void readFromLmSensors();
    void readFromHwmon();

    QTimer* m_timer;
    mutable QMutex m_mutex;

    QVector<TemperatureSensor> m_temperatures;
    QVector<VoltageSensor> m_voltages;
    QVector<FanSensor> m_fans;
};

#endif
