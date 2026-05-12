#ifndef SYSTEMOVERVIEWWIDGET_H
#define SYSTEMOVERVIEWWIDGET_H

#include <QWidget>
#include <QTextBrowser>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QPushButton>
#include <QSplitter>
#include <QLabel>
#include <QVector>
#include <QColor>
#include "systemmodel.h"

struct HardwareNode;

class SystemOverviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SystemOverviewWidget(QWidget *parent = nullptr);

    QTextBrowser* textHardware() const { return m_textHardware; }
    QTextBrowser* textSoftware() const { return m_textSoftware; }
    QTextBrowser* textSensor() const { return m_textSensor; }
    QGraphicsScene* scene() const { return m_scene; }
    QGraphicsView* topologyView() const { return m_topologyView; }

    void updateSystemInfo(double cpuUsage, qulonglong memTotal, qulonglong memAvail);
    void updateTopology(const QVector<HardwareNode>& nodes);
    void setTopologyScene(QGraphicsScene *scene);
    void exportTopologyAsImage(const QString& filePath);

private:
    void initUI();
    void buildTopology(const QVector<HardwareNode>& nodes);
    void setupTopologyControls();
    void parseSensorsOutput(const QString& output);
    void readHwmonTemperatures();
    void readHwmonFans();
    void readHwmonVoltages();
    void readHwmonSensors();

    QTextBrowser *m_textHardware;
    QTextBrowser *m_textSoftware;
    QTextBrowser *m_textSensor;
    QGraphicsScene *m_scene;
    QGraphicsView *m_topologyView;
};

#endif
