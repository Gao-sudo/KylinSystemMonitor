// resourcemonitorwidget.cpp
#include "resourcemonitorwidget.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QtCharts/QChart>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QColor>
#include <QPen>
#include <QDebug>

ResourceMonitorWidget::ResourceMonitorWidget(QWidget *parent)
    : QWidget(parent)
    , m_cpuCard(nullptr)
    , m_memCard(nullptr)
    , m_netCard(nullptr)
    , m_diskCard(nullptr)
{
    initUI();
}

QString ResourceMonitorWidget::getHeatmapGradientStyle(double value)
{
    value = qBound(0.0, value, 100.0);

    int r, g, b;

    if (value < 50) {
        double t = value / 50.0;
        r = (int)(180 + (220 - 180) * t);
        g = (int)(210 + (210 - 210) * t);
        b = (int)(180 + (150 - 180) * t);
    } else {
        double t = (value - 50.0) / 50.0;
        r = (int)(220 + (210 - 220) * t);
        g = (int)(210 + (150 - 210) * t);
        b = (int)(150 + (140 - 150) * t);
    }

    return QString(R"(
        QWidget {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1,
                stop:0 rgba(%1,%2,%3,0.88),
                stop:1 rgba(%1,%2,%3,0.78));
            border-radius: 12px;
            padding: 5px;
        }
    )").arg(r).arg(g).arg(b);
}

QString ResourceMonitorWidget::getHeatmapBarColor(double value)
{
    value = qBound(0.0, value, 100.0);

    if (value < 30) {
        return "#5ba0a0";
    } else if (value < 60) {
        return "#c9a03d";
    } else if (value < 85) {
        return "#c07a5a";
    } else {
        return "#b85c5c";
    }
}

void ResourceMonitorWidget::initUI()
{
    setStyleSheet("ResourceMonitorWidget { background-color: #f8f9fa; }");
    QVBoxLayout *layChartMain = new QVBoxLayout(this);
    layChartMain->setSpacing(15);
    layChartMain->setContentsMargins(15, 15, 15, 15);

    QGroupBox *realtimeInfoBox = new QGroupBox;
    realtimeInfoBox->setTitle("📊 实时资源状态");
    realtimeInfoBox->setStyleSheet(R"(
        QGroupBox {
            background-color: white;
            border: 1px solid #e0e0e0;
            border-radius: 10px;
            margin-top: 10px;
            padding-top: 15px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 20px;
            padding: 0 10px 0 10px;
            color: #2c3e50;
            font-size: 14px;
            font-weight: bold;
        }
    )");

    QGridLayout *realtimeInfoLayout = new QGridLayout(realtimeInfoBox);
    realtimeInfoLayout->setSpacing(20);
    realtimeInfoLayout->setContentsMargins(20, 20, 20, 20);

    // CPU 卡片
    m_cpuCard = new QWidget;
    m_cpuCard->setStyleSheet(getHeatmapGradientStyle(0));
    QHBoxLayout *cpuLayout = new QHBoxLayout(m_cpuCard);
    cpuLayout->setContentsMargins(15,15,15,15);
    QLabel *cpuIcon = new QLabel("🖥️");
    cpuIcon->setStyleSheet("font-size:28px;background:transparent;");
    cpuIcon->setFixedSize(45,45);
    cpuIcon->setAlignment(Qt::AlignCenter);

    QWidget *cpuRight = new QWidget;
    QVBoxLayout *cpuRightLayout = new QVBoxLayout(cpuRight);
    cpuRightLayout->setSpacing(6);
    QLabel *cpuTitle = new QLabel("CPU 使用率");
    cpuTitle->setStyleSheet("color:white;font-size:13px;font-weight:bold;background:transparent;");
    m_cpuInfoLabel = new QLabel("0%");
    m_cpuInfoLabel->setStyleSheet("color:white;font-size:26px;font-weight:bold;background:transparent;");
    m_cpuProgressBar = new QProgressBar;
    m_cpuProgressBar->setRange(0,100);
    m_cpuProgressBar->setFixedHeight(6);
    m_cpuProgressBar->setTextVisible(false);
    m_cpuProgressBar->setStyleSheet(R"(
        QProgressBar { border:none; background:rgba(255,255,255,0.25); border-radius:3px; }
        QProgressBar::chunk { background:white; border-radius:3px; }
    )");

    cpuRightLayout->addWidget(cpuTitle);
    cpuRightLayout->addWidget(m_cpuInfoLabel);
    cpuRightLayout->addWidget(m_cpuProgressBar);
    cpuLayout->addWidget(cpuIcon);
    cpuLayout->addWidget(cpuRight,1);

    // 内存卡片
    m_memCard = new QWidget;
    m_memCard->setStyleSheet(getHeatmapGradientStyle(0));
    QHBoxLayout *memLayout = new QHBoxLayout(m_memCard);
    memLayout->setContentsMargins(15,15,15,15);
    QLabel *memIcon = new QLabel("💾");
    memIcon->setStyleSheet("font-size:28px;background:transparent;");
    memIcon->setFixedSize(45,45);
    memIcon->setAlignment(Qt::AlignCenter);

    QWidget *memRight = new QWidget;
    QVBoxLayout *memRightLayout = new QVBoxLayout(memRight);
    memRightLayout->setSpacing(6);
    QLabel *memTitle = new QLabel("内存使用率");
    memTitle->setStyleSheet("color:white;font-size:13px;font-weight:bold;background:transparent;");
    m_memInfoLabel = new QLabel("0%");
    m_memInfoLabel->setStyleSheet("color:white;font-size:26px;font-weight:bold;background:transparent;");
    m_memProgressBar = new QProgressBar;
    m_memProgressBar->setRange(0,100);
    m_memProgressBar->setFixedHeight(6);
    m_memProgressBar->setTextVisible(false);
    m_memProgressBar->setStyleSheet(R"(
        QProgressBar { border:none; background:rgba(255,255,255,0.25); border-radius:3px; }
        QProgressBar::chunk { background:white; border-radius:3px; }
    )");

    memRightLayout->addWidget(memTitle);
    memRightLayout->addWidget(m_memInfoLabel);
    memRightLayout->addWidget(m_memProgressBar);
    memLayout->addWidget(memIcon);
    memLayout->addWidget(memRight,1);

    // 网络卡片
    m_netCard = new QWidget;
    m_netCard->setStyleSheet(getHeatmapGradientStyle(0));
    QHBoxLayout *netLayout = new QHBoxLayout(m_netCard);
    netLayout->setContentsMargins(15,15,15,15);
    QLabel *netIcon = new QLabel("🌐");
    netIcon->setStyleSheet("font-size:28px;background:transparent;");
    netIcon->setFixedSize(45,45);
    netIcon->setAlignment(Qt::AlignCenter);

    QWidget *netRight = new QWidget;
    QVBoxLayout *netRightLayout = new QVBoxLayout(netRight);
    netRightLayout->setSpacing(4);
    QLabel *netTitle = new QLabel("网络速率");
    netTitle->setStyleSheet("color:white;font-size:13px;font-weight:bold;background:transparent;");
    m_netInfoLabel = new QLabel;
    m_netInfoLabel->setStyleSheet("color:white;font-size:12px;font-weight:bold;background:transparent;");

    netRightLayout->addWidget(netTitle);
    netRightLayout->addWidget(m_netInfoLabel);
    netLayout->addWidget(netIcon);
    netLayout->addWidget(netRight,1);

    // 磁盘卡片
    m_diskCard = new QWidget;
    m_diskCard->setStyleSheet(getHeatmapGradientStyle(0));
    QHBoxLayout *diskLayout = new QHBoxLayout(m_diskCard);
    diskLayout->setContentsMargins(15,15,15,15);
    QLabel *diskIcon = new QLabel("💿");
    diskIcon->setStyleSheet("font-size:28px;background:transparent;");
    diskIcon->setFixedSize(45,45);
    diskIcon->setAlignment(Qt::AlignCenter);

    QWidget *diskRight = new QWidget;
    QVBoxLayout *diskRightLayout = new QVBoxLayout(diskRight);
    diskRightLayout->setSpacing(4);
    QLabel *diskTitle = new QLabel("磁盘读写");
    diskTitle->setStyleSheet("color:white;font-size:13px;font-weight:bold;background:transparent;");
    m_diskInfoLabel = new QLabel;
    m_diskInfoLabel->setStyleSheet("color:white;font-size:12px;font-weight:bold;background:transparent;");

    diskRightLayout->addWidget(diskTitle);
    diskRightLayout->addWidget(m_diskInfoLabel);
    diskLayout->addWidget(diskIcon);
    diskLayout->addWidget(diskRight,1);

    realtimeInfoLayout->addWidget(m_cpuCard, 0, 0);
    realtimeInfoLayout->addWidget(m_memCard, 0, 1);
    realtimeInfoLayout->addWidget(m_netCard, 1, 0);
    realtimeInfoLayout->addWidget(m_diskCard, 1, 1);
    realtimeInfoLayout->setColumnStretch(0,1);
    realtimeInfoLayout->setColumnStretch(1,1);

    layChartMain->addWidget(realtimeInfoBox);

    initCharts();
    layChartMain->addWidget(m_chartView, 1);
}

void ResourceMonitorWidget::initCharts()
{
    m_cpuSeries = new QLineSeries();
    m_memSeries = new QLineSeries();
    m_netRxSeries = new QLineSeries();
    m_netTxSeries = new QLineSeries();
    m_diskReadSeries = new QLineSeries();
    m_diskWriteSeries = new QLineSeries();

    m_cpuSeries->setName("CPU 使用率(%)");
    m_memSeries->setName("内存使用率(%)");
    m_netRxSeries->setName("网络接收(MB/s)");
    m_netTxSeries->setName("网络发送(MB/s)");
    m_diskReadSeries->setName("磁盘读取(MB/s)");
    m_diskWriteSeries->setName("磁盘写入(MB/s)");

    m_cpuSeries->setColor(Qt::red);
    m_memSeries->setColor(Qt::blue);
    m_netRxSeries->setColor(Qt::green);
    m_netTxSeries->setColor(Qt::cyan);
    m_diskReadSeries->setColor(Qt::magenta);
    m_diskWriteSeries->setColor(QColor(255,140,0));

    m_resourceChart = new QChart();
    m_resourceChart->addSeries(m_cpuSeries);
    m_resourceChart->addSeries(m_memSeries);
    m_resourceChart->addSeries(m_netRxSeries);
    m_resourceChart->addSeries(m_netTxSeries);
    m_resourceChart->addSeries(m_diskReadSeries);
    m_resourceChart->addSeries(m_diskWriteSeries);

    m_resourceChart->setTitle("实时资源监控");
    m_resourceChart->legend()->setVisible(true);
    m_resourceChart->setAnimationOptions(QChart::SeriesAnimations);

    m_axisX = new QValueAxis();
    m_axisY = new QValueAxis();
    m_axisX->setRange(0,60);
    m_axisY->setRange(0,100);
    m_axisY->setTitleText("百分比 / 速率(MB/s)");
    m_axisX->setTitleText("时间点 (最近60秒)");

    m_resourceChart->addAxis(m_axisX, Qt::AlignBottom);
    m_resourceChart->addAxis(m_axisY, Qt::AlignLeft);

    m_cpuSeries->attachAxis(m_axisX);
    m_cpuSeries->attachAxis(m_axisY);
    m_memSeries->attachAxis(m_axisX);
    m_memSeries->attachAxis(m_axisY);
    m_netRxSeries->attachAxis(m_axisX);
    m_netRxSeries->attachAxis(m_axisY);
    m_netTxSeries->attachAxis(m_axisX);
    m_netTxSeries->attachAxis(m_axisY);
    m_diskReadSeries->attachAxis(m_axisX);
    m_diskReadSeries->attachAxis(m_axisY);
    m_diskWriteSeries->attachAxis(m_axisX);
    m_diskWriteSeries->attachAxis(m_axisY);

    m_chartView = new QChartView(m_resourceChart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(350);
    m_chartView->setStyleSheet(R"(
        QChartView {
            background-color:white;
            border:1px solid #e0e0e0;
            border-radius:10px;
        }
    )");
}

void ResourceMonitorWidget::updateRealtimeData(double cpuUsage, double memUsage,
                                              double netRx, double netTx,
                                              double diskRead, double diskWrite)
{
    // CPU
    m_cpuInfoLabel->setText(QString::number(cpuUsage, 'f', 1) + "%");
    m_cpuProgressBar->setValue(qRound(cpuUsage));

    // 内存
    m_memInfoLabel->setText(QString::number(memUsage, 'f', 1) + "%");
    m_memProgressBar->setValue(qRound(memUsage));

    // 网络
    m_netInfoLabel->setText(QString("↓ %1 MB/s    ↑ %2 MB/s")
                            .arg(netRx, 0, 'f', 2).arg(netTx, 0, 'f', 2));

    // 磁盘
    m_diskInfoLabel->setText(QString("读 %1 MB/s    写 %2 MB/s")
                             .arg(diskRead, 0, 'f', 2).arg(diskWrite, 0, 'f', 2));
}
void ResourceMonitorWidget::updateResource(double cpuUsage, double memTotal, double memAvail,
                                          double diskRead, double diskWrite,
                                          double netRx, double netTx)
{
    double memUsage = 0.0;
    if (memTotal > 0)
        memUsage = 100.0 * (memTotal - memAvail) / memTotal;

    updateRealtimeData(cpuUsage, memUsage, netRx, netTx, diskRead, diskWrite);
}


// resourcemonitorwidget.cpp
void ResourceMonitorWidget::updateCharts(const QVector<double>& cpuHistory,
                                          const QVector<double>& memHistory,
                                          const QVector<double>& netRxHistory,
                                          const QVector<double>& netTxHistory,
                                          const QVector<double>& diskReadHistory,
                                          const QVector<double>& diskWriteHistory)
{
    // 清空现有数据
    m_cpuSeries->clear();
    m_memSeries->clear();
    m_netRxSeries->clear();
    m_netTxSeries->clear();
    m_diskReadSeries->clear();
    m_diskWriteSeries->clear();

    // 调试输出
    qDebug() << "updateCharts - diskReadHistory size:" << diskReadHistory.size();
    if (diskReadHistory.size() > 0) {
        qDebug() << "  last diskRead:" << diskReadHistory.last();
    }

    // 添加磁盘数据（单独处理，确保显示）
    for (int i = 0; i < diskReadHistory.size(); ++i) {
        m_diskReadSeries->append(i, diskReadHistory[i]);
        m_diskWriteSeries->append(i, diskWriteHistory[i]);
    }

    // 添加其他数据
    for (int i = 0; i < cpuHistory.size(); ++i) {
        m_cpuSeries->append(i, cpuHistory[i]);
        m_memSeries->append(i, memHistory[i]);
    }

    for (int i = 0; i < netRxHistory.size(); ++i) {
        m_netRxSeries->append(i, netRxHistory[i]);
        m_netTxSeries->append(i, netTxHistory[i]);
    }

    // 调整 Y 轴范围（考虑磁盘读写）
    if (m_axisY) {
        double maxValue = 0;
        for (double v : diskReadHistory) if (v > maxValue) maxValue = v;
        for (double v : diskWriteHistory) if (v > maxValue) maxValue = v;
        maxValue = qMax(maxValue, 100.0);
        m_axisY->setRange(0, maxValue * 1.1);
    }
}
