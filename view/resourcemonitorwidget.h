#ifndef RESOURCEMONITORWIDGET_H
#define RESOURCEMONITORWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QProgressBar>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>

QT_CHARTS_USE_NAMESPACE

class ResourceMonitorWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ResourceMonitorWidget(QWidget *parent = nullptr);

    QLabel* cpuInfoLabel()    const { return m_cpuInfoLabel; }
    QLabel* memInfoLabel()    const { return m_memInfoLabel; }
    QLabel* netInfoLabel()    const { return m_netInfoLabel; }
    QLabel* diskInfoLabel()   const { return m_diskInfoLabel; }
    QProgressBar* cpuProgressBar() const { return m_cpuProgressBar; }
    QProgressBar* memProgressBar() const { return m_memProgressBar; }

    QChartView* chartView()   const { return m_chartView; }
    QLineSeries* cpuSeries()  const { return m_cpuSeries; }
    QLineSeries* memSeries()  const { return m_memSeries; }
    QLineSeries* netRxSeries() const { return m_netRxSeries; }
    QLineSeries* netTxSeries() const { return m_netTxSeries; }
    QLineSeries* diskReadSeries() const { return m_diskReadSeries; }
    QLineSeries* diskWriteSeries() const { return m_diskWriteSeries; }
    QValueAxis* axisX()       const { return m_axisX; }
    QValueAxis* axisY()       const { return m_axisY; }

    void updateRealtimeData(double cpu, double mem, double netRx, double netTx, double diskRd, double diskWr);

    void updateCharts(const QVector<double>& cpuHistory,
                      const QVector<double>& memHistory,
                      const QVector<double>& netRxHistory,
                      const QVector<double>& netTxHistory,
                      const QVector<double>& diskReadHistory,
                      const QVector<double>& diskWriteHistory);

public slots:
    void updateResource(double cpu, double memTotal, double memAvail,
                        double diskRead, double diskWrite,
                        double netRx, double netTx);


private:
    void initUI();
    void initCharts();

    // 柔和热力图辅助函数
    QString getHeatmapGradientStyle(double value);
    QString getHeatmapBarColor(double value);

    QLabel *m_cpuInfoLabel;
    QLabel *m_memInfoLabel;
    QLabel *m_netInfoLabel;
    QLabel *m_diskInfoLabel;
    QProgressBar *m_cpuProgressBar;
    QProgressBar *m_memProgressBar;

    QWidget *m_cpuCard;
    QWidget *m_memCard;
    QWidget *m_netCard;
    QWidget *m_diskCard;

    QChartView *m_chartView;
    QChart *m_resourceChart;
    QLineSeries *m_cpuSeries;
    QLineSeries *m_memSeries;
    QLineSeries *m_netRxSeries;
    QLineSeries *m_netTxSeries;
    QLineSeries *m_diskReadSeries;
    QLineSeries *m_diskWriteSeries;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;

    int m_pointCount;
};

#endif
