#ifndef PROCESSREPORTDIALOG_H
#define PROCESSREPORTDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>

class ProcessReportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProcessReportDialog(int pid, const QString &processName, QWidget *parent = nullptr);
    ~ProcessReportDialog();

private slots:
    void updateData();
    void exportToCsv();
    void exportToJson();

private:
    void setupUI();
    void collectProcessData();

    int m_pid;
    QString m_processName;
    QTimer *m_timer;

    QtCharts::QChartView *m_chartView;
    QtCharts::QLineSeries *m_cpuSeries;
    QtCharts::QLineSeries *m_memSeries;
    QtCharts::QValueAxis *m_axisX;
    QtCharts::QValueAxis *m_axisY;

    QVector<double> m_cpuHistory;
    QVector<double> m_memHistory;
    int m_pointCount;
};

#endif
