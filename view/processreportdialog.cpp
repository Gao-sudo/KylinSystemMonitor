#include "processreportdialog.h"
#include "../model/processmodel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QDir>

ProcessReportDialog::ProcessReportDialog(int pid, const QString &processName, QWidget *parent)
    : QDialog(parent)
    , m_pid(pid)
    , m_processName(processName)
    , m_timer(nullptr)
    , m_chartView(nullptr)
    , m_cpuSeries(nullptr)
    , m_memSeries(nullptr)
    , m_axisX(nullptr)
    , m_axisY(nullptr)
    , m_pointCount(0)
{
    setWindowTitle(QString("Process Report - %1 (PID: %2)").arg(processName).arg(pid));
    setMinimumSize(700, 500);
    setupUI();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &ProcessReportDialog::updateData);
    m_timer->start(1000);
}

ProcessReportDialog::~ProcessReportDialog()
{
    if (m_timer) m_timer->stop();
}

void ProcessReportDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 进程信息
    QLabel *infoLabel = new QLabel(QString("<b>Process:</b> %1 (PID: %2)").arg(m_processName).arg(m_pid));
    infoLabel->setStyleSheet("font-size: 14px; padding: 5px;");
    mainLayout->addWidget(infoLabel);

    // 图表
    m_chartView = new QtCharts::QChartView(this);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(350);

    QtCharts::QChart *chart = new QtCharts::QChart();
    chart->setTitle("CPU / Memory Usage Trend");
    chart->legend()->setVisible(true);

    m_cpuSeries = new QtCharts::QLineSeries();
    m_cpuSeries->setName("CPU Usage (%)");
    m_cpuSeries->setColor(Qt::red);

    m_memSeries = new QtCharts::QLineSeries();
    m_memSeries->setName("Memory Usage (MB)");
    m_memSeries->setColor(Qt::blue);

    chart->addSeries(m_cpuSeries);
    chart->addSeries(m_memSeries);

    m_axisX = new QtCharts::QValueAxis();
    m_axisX->setRange(0, 60);
    m_axisX->setTitleText("Time (seconds)");

    m_axisY = new QtCharts::QValueAxis();
    m_axisY->setRange(0, 100);
    m_axisY->setTitleText("Value");

    chart->addAxis(m_axisX, Qt::AlignBottom);
    chart->addAxis(m_axisY, Qt::AlignLeft);

    m_cpuSeries->attachAxis(m_axisX);
    m_cpuSeries->attachAxis(m_axisY);
    m_memSeries->attachAxis(m_axisX);
    m_memSeries->attachAxis(m_axisY);

    m_chartView->setChart(chart);
    mainLayout->addWidget(m_chartView);

    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *btnExportCsv = new QPushButton("Export CSV");
    QPushButton *btnExportJson = new QPushButton("Export JSON");
    QPushButton *btnClose = new QPushButton("Close");

    connect(btnExportCsv, &QPushButton::clicked, this, &ProcessReportDialog::exportToCsv);
    connect(btnExportJson, &QPushButton::clicked, this, &ProcessReportDialog::exportToJson);
    connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);

    buttonLayout->addWidget(btnExportCsv);
    buttonLayout->addWidget(btnExportJson);
    buttonLayout->addStretch();
    buttonLayout->addWidget(btnClose);

    mainLayout->addLayout(buttonLayout);
}

void ProcessReportDialog::updateData()
{
    collectProcessData();

    // 更新图表
    m_cpuSeries->clear();
    m_memSeries->clear();

    for (int i = 0; i < m_cpuHistory.size(); ++i) {
        m_cpuSeries->append(i, m_cpuHistory[i]);
        m_memSeries->append(i, m_memHistory[i]);
    }

    // 调整X轴范围
    if (m_cpuHistory.size() >= 60) {
        m_axisX->setRange(m_cpuHistory.size() - 60, m_cpuHistory.size());
    } else {
        m_axisX->setRange(0, 60);
    }

    // 调整Y轴范围
    double maxValue = 0;
    for (double v : m_cpuHistory) if (v > maxValue) maxValue = v;
    for (double v : m_memHistory) if (v > maxValue) maxValue = v;
    m_axisY->setRange(0, qMax(maxValue + 10, 100.0));
}

void ProcessReportDialog::collectProcessData()
{
    ProcessModel* model = ProcessModel::instance();
    auto info = model->getProcessById(m_pid);

    if (info.pid != -1) {
        m_cpuHistory.append(info.cpu);
        m_memHistory.append(info.mem);

        // 保持最近60个点
        while (m_cpuHistory.size() > 60) {
            m_cpuHistory.removeFirst();
            m_memHistory.removeFirst();
        }
    }
}

void ProcessReportDialog::exportToCsv()
{
    QString path = QFileDialog::getSaveFileName(this, "Export CSV",
                                                QDir::homePath() + "/process_report.csv",
                                                "CSV (*.csv)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Cannot save file");
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out.setGenerateByteOrderMark(true);

    out << "Time,CPU(%),Memory(MB)\n";
    for (int i = 0; i < m_cpuHistory.size(); ++i) {
        out << i << "," << m_cpuHistory[i] << "," << m_memHistory[i] << "\n";
    }

    file.close();
    QMessageBox::information(this, "Success", "CSV saved to:\n" + path);
}

void ProcessReportDialog::exportToJson()
{
    QString path = QFileDialog::getSaveFileName(this, "Export JSON",
                                                QDir::homePath() + "/process_report.json",
                                                "JSON (*.json)");
    if (path.isEmpty()) return;

    QJsonObject root;
    root["pid"] = m_pid;
    root["process_name"] = m_processName;
    root["export_time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");

    QJsonArray history;
    for (int i = 0; i < m_cpuHistory.size(); ++i) {
        QJsonObject point;
        point["time"] = i;
        point["cpu"] = m_cpuHistory[i];
        point["mem_mb"] = m_memHistory[i];
        history.append(point);
    }
    root["history"] = history;

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
        QMessageBox::information(this, "Success", "JSON saved to:\n" + path);
    } else {
        QMessageBox::warning(this, "Error", "Cannot save file");
    }
}
