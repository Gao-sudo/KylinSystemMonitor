// systemoverviewwidget.cpp
#include "systemoverviewwidget.h"
#include "systemmodel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QProcess>
#include <QDir>
#include <QSysInfo>
#include <QThread>
#include <QDateTime>
#include <QLabel>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsTextItem>
#include <QGraphicsLineItem>
#include <QFile>
#include <QRegularExpression>
#include <QPainter>
#include <QContextMenuEvent>

SystemOverviewWidget::SystemOverviewWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}
void SystemOverviewWidget::initUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    QWidget *infoContainer = new QWidget;
    QHBoxLayout *infoLayout = new QHBoxLayout(infoContainer);

    m_textHardware = new QTextBrowser;
    m_textSoftware = new QTextBrowser;
    m_textSensor = new QTextBrowser;

    m_textHardware->setMinimumHeight(200);
    m_textSoftware->setMinimumHeight(200);
    m_textSensor->setMinimumHeight(200);

    // ========== 复制必生效的3行 ==========
    m_textHardware->setReadOnly(true);
    m_textHardware->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    m_textHardware->setContextMenuPolicy(Qt::DefaultContextMenu);

    m_textSoftware->setReadOnly(true);
    m_textSoftware->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    m_textSoftware->setContextMenuPolicy(Qt::DefaultContextMenu);

    m_textSensor->setReadOnly(true);
    m_textSensor->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    m_textSensor->setContextMenuPolicy(Qt::DefaultContextMenu);
    // ====================================

    infoLayout->addWidget(m_textHardware);
    infoLayout->addWidget(m_textSoftware);
    infoLayout->addWidget(m_textSensor);

    QLabel *topologyTitle = new QLabel("System Hardware Topology");
    topologyTitle->setStyleSheet("font-size: 14px; font-weight: bold; margin-top: 10px;");

    m_scene = new QGraphicsScene(this);
    m_topologyView = new QGraphicsView;
    m_topologyView->setScene(m_scene);
    m_topologyView->setMinimumHeight(400);
    m_topologyView->setRenderHint(QPainter::Antialiasing);
    m_topologyView->setDragMode(QGraphicsView::ScrollHandDrag);

    mainLayout->addWidget(infoContainer);
    mainLayout->addWidget(topologyTitle);
    mainLayout->addWidget(m_topologyView);
}

void SystemOverviewWidget::setTopologyScene(QGraphicsScene *scene)
{
    if (m_scene != scene) {
        delete m_scene;
        m_scene = scene;
        m_topologyView->setScene(m_scene);
    }
}

void SystemOverviewWidget::updateTopology(const QVector<HardwareNode>& nodes)
{
    buildTopology(nodes);
}

void SystemOverviewWidget::buildTopology(const QVector<HardwareNode>& nodes)
{
    m_scene->clear();

    if (nodes.isEmpty()) {
        m_scene->addText("No hardware nodes detected");
        return;
    }

    // 计算场景大小
    int sceneWidth = 1200;
    int sceneHeight = 600;
    m_scene->setSceneRect(0, 0, sceneWidth, sceneHeight);

    int nodeWidth = 160;
    int nodeHeight = 55;
    int levelHeight = 100;
    int startX = 50;
    int startY = 30;

    // 按层级分组
    QMap<int, QVector<int>> levelMap;
    for (int i = 0; i < nodes.size(); ++i) {
        levelMap[nodes[i].level].append(i);
    }

    // 存储每个节点的位置
    QMap<int, QPointF> positions;

    // 计算每层节点的X位置（居中显示）
    for (auto it = levelMap.begin(); it != levelMap.end(); ++it) {
        int level = it.key();
        const QVector<int>& indices = it.value();

        int totalWidth = indices.size() * nodeWidth;
        int levelStartX = (sceneWidth - totalWidth) / 2;

        int x = levelStartX;
        int y = startY + level * levelHeight;

        for (int idx : indices) {
            positions[idx] = QPointF(x, y);
            x += nodeWidth + 20;
        }
    }

    // 绘制连线（先画线，再画节点）
    QPen linePen(Qt::darkGray, 2, Qt::SolidLine);

    for (int i = 0; i < nodes.size(); ++i) {
        const HardwareNode& node = nodes[i];
        if (node.parentId >= 0 && positions.contains(i) && positions.contains(node.parentId)) {
            QPointF childCenter = positions[i] + QPointF(nodeWidth/2, nodeHeight/2);
            QPointF parentCenter = positions[node.parentId] + QPointF(nodeWidth/2, nodeHeight/2);
            m_scene->addLine(parentCenter.x(), parentCenter.y(),
                            childCenter.x(), childCenter.y(), linePen);
        }
    }

    // 绘制节点
    QFont nodeFont("Arial", 9, QFont::Bold);
    QFont detailFont("Arial", 7);
    QPen nodePen(Qt::white, 2);

    for (int i = 0; i < nodes.size(); ++i) {
        const HardwareNode& node = nodes[i];
        if (!positions.contains(i)) continue;

        QPointF pos = positions[i];

        // 绘制圆角矩形
        m_scene->addRect(pos.x(), pos.y(), nodeWidth, nodeHeight,
                        nodePen, QBrush(node.color));

        // 绘制名称
        QGraphicsTextItem *text = m_scene->addText(node.name, nodeFont);
        text->setDefaultTextColor(Qt::white);
        text->setPos(pos.x() + 10, pos.y() + 12);

        // 绘制详细信息
        if (!node.detail.isEmpty()) {
            QString shortDetail = node.detail;
            if (shortDetail.length() > 25) {
                shortDetail = shortDetail.left(22) + "...";
            }
            QGraphicsTextItem *detail = m_scene->addText(shortDetail, detailFont);
            detail->setDefaultTextColor(QColor(255, 255, 255, 200));
            detail->setPos(pos.x() + 10, pos.y() + 32);
        }

        // 添加图标
        QString icon;
        if (node.type == "system") icon = "🖥️";
        else if (node.type == "cpu") icon = "⚡";
        else if (node.type == "core") icon = "🔘";
        else if (node.type == "memory") icon = "💾";
        else if (node.type == "gpu") icon = "🎮";
        else if (node.type == "disk_root") icon = "💿";
        else if (node.type == "partition") icon = "📁";
        else if (node.type == "net_root") icon = "🌐";
        else if (node.type == "interface") icon = "🔌";
        else icon = "📦";

        QGraphicsTextItem *iconText = m_scene->addText(icon);
        iconText->setFont(QFont("Segoe UI Emoji", 14));
        iconText->setDefaultTextColor(Qt::white);
        iconText->setPos(pos.x() + nodeWidth - 28, pos.y() + 18);

        // 添加工具提示
        QString tooltip = QString("%1\n%2").arg(node.name).arg(node.detail);
        iconText->setToolTip(tooltip);
        text->setToolTip(tooltip);
    }

    // 添加标题
    QGraphicsTextItem *title = m_scene->addText("Hardware Topology");
    title->setFont(QFont("Arial", 14, QFont::Bold));
    title->setDefaultTextColor(QColor(44, 62, 80));
    title->setPos((sceneWidth - title->boundingRect().width()) / 2, 5);

    // 添加图例
    int legendY = sceneHeight - 50;
    int legendX = 20;

    auto addLegend = [this](int x, int y, QColor color, QString text) {
        m_scene->addRect(x, y, 15, 15, QPen(Qt::gray), QBrush(color));
        QGraphicsTextItem *label = m_scene->addText(text);
        label->setPos(x + 20, y - 2);
        label->setFont(QFont("Arial", 8));
    };

    addLegend(legendX, legendY, QColor(46, 204, 113), "CPU");
    addLegend(legendX + 80, legendY, QColor(241, 196, 15), "Memory");
    addLegend(legendX + 160, legendY, QColor(231, 76, 60), "GPU");
    addLegend(legendX + 240, legendY, QColor(155, 89, 182), "Disk");
    addLegend(legendX + 320, legendY, QColor(230, 126, 34), "Network");
}

void SystemOverviewWidget::updateSystemInfo(double cpuUsage, qulonglong memTotal, qulonglong memAvail)
{
    // ========== 硬件信息栏 ==========
    m_textHardware->clear();

    // CPU 信息
    m_textHardware->append("<b>===== CPU Information =====</b>");
    QFile cpu("/proc/cpuinfo");
    if (cpu.open(QIODevice::ReadOnly)) {
        QString s = QString::fromUtf8(cpu.readAll());
        QStringList lines = s.split('\n');
        for (const QString& line : lines) {
            if (line.startsWith("model name")) {
                QString model = line.split(":").value(1).trimmed();
                m_textHardware->append("Model : " + model);
                break;
            }
        }
        m_textHardware->append("Architecture : " + QSysInfo::currentCpuArchitecture());
        m_textHardware->append("Cores : " + QString::number(QThread::idealThreadCount()));
        m_textHardware->append(QString("Current Usage : %1%").arg(cpuUsage, 0, 'f', 1));

        // 读取 CPU 频率
        QFile cpuinfo("/proc/cpuinfo");
        if (cpuinfo.open(QIODevice::ReadOnly)) {
            QString content = QString::fromUtf8(cpuinfo.readAll());
            cpuinfo.close();
            if (content.contains("cpu MHz")) {
                QStringList lines2 = content.split('\n');
                for (const QString& line : lines2) {
                    if (line.startsWith("cpu MHz")) {
                        QString freq = line.split(":").value(1).trimmed();
                        m_textHardware->append("Frequency : " + freq + " MHz");
                        break;
                    }
                }
            }
        }
        cpu.close();
    }

    // 内存信息
    m_textHardware->append("<b>===== Memory Information =====</b>");
    if (memTotal > 0) {
        double totalMB = memTotal / 1024.0;
        double availMB = memAvail / 1024.0;
        double usedMB = totalMB - availMB;
        double usedPercent = 100.0 * usedMB / totalMB;
        m_textHardware->append(QString("Total : %1 MB (%2 GB)").arg(totalMB, 0, 'f', 0).arg(totalMB/1024, 0, 'f', 2));
        m_textHardware->append(QString("Used : %1 MB (%2 GB)").arg(usedMB, 0, 'f', 0).arg(usedMB/1024, 0, 'f', 2));
        m_textHardware->append(QString("Available : %1 MB (%2 GB)").arg(availMB, 0, 'f', 0).arg(availMB/1024, 0, 'f', 2));
        m_textHardware->append(QString("Usage : %1%").arg(usedPercent, 0, 'f', 1));
    }

    // 尝试读取内存型号（需要 sudo）
    m_textHardware->append("<b>===== Memory Modules =====</b>");
    QProcess dmidecode;
    dmidecode.start("sudo", QStringList() << "dmidecode" << "-t" << "memory");
    if (dmidecode.waitForFinished(3000)) {
        QString output = QString::fromUtf8(dmidecode.readAllStandardOutput());
        if (output.contains("Size:")) {
            QStringList lines = output.split('\n');
            bool hasMem = false;
            for (const QString& line : lines) {
                if (line.contains("Size:") && !line.contains("No Module")) {
                    QString size = line.split(":").value(1).trimmed();
                    m_textHardware->append("  " + size);
                    hasMem = true;
                }
                if (line.contains("Type:") && line.contains("DDR")) {
                    QString type = line.split(":").value(1).trimmed();
                    m_textHardware->append("    Type: " + type);
                }
                if (line.contains("Speed:") && line.contains("MHz")) {
                    QString speed = line.split(":").value(1).trimmed();
                    m_textHardware->append("    Speed: " + speed);
                }
                if (line.contains("Manufacturer:")) {
                    QString manu = line.split(":").value(1).trimmed();
                    if (!manu.isEmpty() && manu != "Not Specified") {
                        m_textHardware->append("    Manufacturer: " + manu);
                    }
                }
            }
            if (!hasMem) m_textHardware->append("  Run with 'sudo' to see memory details");
        } else {
            m_textHardware->append("  Run 'sudo dmidecode' to see memory details");
        }
    } else {
        m_textHardware->append("  Run with 'sudo' to see memory module details");
    }

    // 磁盘信息
    m_textHardware->append("<b>===== Disk Information =====</b>");
    QProcess df;
    df.start("df", QStringList() << "-h" << "/");
    if (df.waitForFinished(2000)) {
        QString output = QString::fromUtf8(df.readAllStandardOutput());
        QStringList lines = output.split('\n');
        if (lines.size() >= 2) {
            QStringList parts = lines[1].split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (parts.size() >= 6) {
                m_textHardware->append(QString("Filesystem : %1").arg(parts[0]));
                m_textHardware->append(QString("Total : %1").arg(parts[1]));
                m_textHardware->append(QString("Used : %1").arg(parts[2]));
                m_textHardware->append(QString("Available : %1").arg(parts[3]));
                m_textHardware->append(QString("Usage : %1").arg(parts[4]));
                m_textHardware->append(QString("Mount : %1").arg(parts[5]));
            }
        }
    }

    // 磁盘 SMART 信息
    m_textHardware->append("<b>===== Disk SMART =====</b>");
    QDir devDir("/dev");
    QStringList devices = devDir.entryList(QDir::System | QDir::NoDotAndDotDot);
    bool hasSmart = false;
    for (const QString& device : devices) {
        if (device.startsWith("sd") || device.startsWith("nvme")) {
            QString devPath = "/dev/" + device;
            QProcess smartctl;
            smartctl.start("sudo", QStringList() << "smartctl" << "-H" << devPath);
            if (smartctl.waitForFinished(3000)) {
                QString output = QString::fromUtf8(smartctl.readAllStandardOutput());
                bool isPassed = output.contains("PASSED");
                QString status = isPassed ? "✅ Good" : "⚠️ Warning";
                m_textHardware->append(QString("  %1 : %2").arg(devPath).arg(status));
                hasSmart = true;
            }
        }
    }
    if (!hasSmart) {
        m_textHardware->append("  Install smartmontools: sudo apt install smartmontools");
    }

    // GPU 信息
    m_textHardware->append("<b>===== GPU Information =====</b>");
    bool hasGpu = false;
    QProcess lspci;
    lspci.start("lspci", QStringList() << "|" << "grep" << "-i" << "vga");
    if (lspci.waitForFinished(2000)) {
        QString output = QString::fromUtf8(lspci.readAllStandardOutput()).trimmed();
        if (!output.isEmpty()) {
            m_textHardware->append(output);
            hasGpu = true;
        }
    }

    // NVIDIA GPU 详细信息
    QProcess nvidiaSmi;
    nvidiaSmi.start("nvidia-smi", QStringList() << "--query-gpu=name,temperature.gpu,utilization.gpu,memory.used,memory.total"
                    << "--format=csv,noheader");
    if (nvidiaSmi.waitForFinished(2000)) {
        QString output = QString::fromUtf8(nvidiaSmi.readAllStandardOutput()).trimmed();
        if (!output.isEmpty()) {
            QStringList parts = output.split(",");
            if (parts.size() >= 5) {
                QString name = parts[0].trimmed();
                double temp = parts[1].trimmed().toDouble();
                double usage = parts[2].trimmed().remove("%").toDouble();
                double memUsed = parts[3].trimmed().remove("MiB").toDouble();
                double memTotal = parts[4].trimmed().remove("MiB").toDouble();
                m_textHardware->append(QString("  Name: %1").arg(name));
                m_textHardware->append(QString("  Temperature: %1°C").arg(temp));
                m_textHardware->append(QString("  GPU Usage: %1%").arg(usage));
                m_textHardware->append(QString("  Memory: %1 / %2 MB").arg(memUsed).arg(memTotal));
                hasGpu = true;
            }
        }
    }

    if (!hasGpu) {
        m_textHardware->append("  No dedicated GPU detected");
    }

    // ========== 软件信息栏 ==========
    m_textSoftware->clear();

    // 内核版本
    m_textSoftware->append("<b>===== Kernel Version =====</b>");
    QFile ver("/proc/version");
    if (ver.open(QIODevice::ReadOnly)) {
        QString verStr = QString::fromUtf8(ver.readAll()).trimmed();
        QStringList parts = verStr.split(' ');
        if (parts.size() >= 3) {
            m_textSoftware->append("Version: " + parts[2]);
        }
        m_textSoftware->append(verStr.left(150));
        ver.close();
    }

    // 系统运行时间
    m_textSoftware->append("<b>===== System Uptime =====</b>");
    QFile uptime("/proc/uptime");
    if (uptime.open(QIODevice::ReadOnly)) {
        double seconds = uptime.readAll().trimmed().split(' ')[0].toDouble();
        int days = static_cast<int>(seconds) / 86400;
        int hours = (static_cast<int>(seconds) % 86400) / 3600;
        int minutes = (static_cast<int>(seconds) % 3600) / 60;
        m_textSoftware->append(QString("%1 days, %2 hours, %3 minutes").arg(days).arg(hours).arg(minutes));
        uptime.close();
    }

    // 系统负载
    m_textSoftware->append("<b>===== System Load =====</b>");
    QFile loadavg("/proc/loadavg");
    if (loadavg.open(QIODevice::ReadOnly)) {
        QString loads = QString::fromUtf8(loadavg.readAll()).trimmed();
        m_textSoftware->append("1min / 5min / 15min : " + loads);
        loadavg.close();
    }

    // 操作系统
    m_textSoftware->append("<b>===== Operating System =====</b>");
    QFile osRelease("/etc/os-release");
    if (osRelease.open(QIODevice::ReadOnly)) {
        QString content = QString::fromUtf8(osRelease.readAll());
        QStringList lines = content.split('\n');
        for (const QString& line : lines) {
            if (line.startsWith("PRETTY_NAME=")) {
                QString name = line.split("=").value(1);
                name.remove("\"");
                m_textSoftware->append(name);
                break;
            }
        }
        osRelease.close();
    }

    // 已加载模块（前15个）
    m_textSoftware->append("<b>===== Loaded Kernel Modules =====</b>");
    QFile modules("/proc/modules");
    if (modules.open(QIODevice::ReadOnly)) {
        QString content = QString::fromUtf8(modules.readAll());
        modules.close();
        QStringList lines = content.split('\n');
        int count = 0;
        for (const QString& line : lines) {
            if (!line.isEmpty() && count < 15) {
                QString name = line.split(' ').value(0);
                m_textSoftware->append("  " + name);
                count++;
            }
        }
        if (lines.size() > 15) {
            m_textSoftware->append(QString("  ... and %1 more").arg(lines.size() - 15));
        }
    }

    // 系统服务状态（前10个运行中的服务）
    m_textSoftware->append("<b>===== System Services =====</b>");
    QProcess systemctl;
    systemctl.start("systemctl", QStringList() << "list-units" << "--type=service"
                    << "--state=running" << "--no-pager" << "--no-legend");
    if (systemctl.waitForFinished(5000)) {
        QString output = QString::fromUtf8(systemctl.readAllStandardOutput());
        QStringList lines = output.split('\n');
        int count = 0;
        for (const QString& line : lines) {
            if (!line.isEmpty() && count < 10) {
                QString name = line.split(' ').value(0);
                m_textSoftware->append("  🟢 " + name);
                count++;
            }
        }
        if (lines.size() > 10) {
            m_textSoftware->append(QString("  ... and %1 more").arg(lines.size() - 10));
        }
    } else {
        m_textSoftware->append("  Run with 'sudo' to see services");
    }

    // ========== 传感器信息栏 ==========
    m_textSensor->clear();
    m_textSensor->append("<b>===== Temperature Sensors =====</b>");

    QProcess sensors;
    sensors.start("sensors", QStringList());
    if (sensors.waitForFinished(2000)) {
        QString output = QString::fromUtf8(sensors.readAllStandardOutput());
        if (!output.isEmpty()) {
            QStringList lines = output.split('\n');
            for (const QString& line : lines) {
                QString trimmed = line.trimmed();
                if (trimmed.contains("°C") && (trimmed.contains("+") || trimmed.contains("-"))) {
                    m_textSensor->append("  " + trimmed);
                }
            }
        } else {
            m_textSensor->append("  No sensors detected");
            m_textSensor->append("  Install: sudo apt install lm-sensors");
            m_textSensor->append("  Configure: sudo sensors-detect");
        }
    } else {
        m_textSensor->append("  sensors command not found");
        m_textSensor->append("  Install: sudo apt install lm-sensors");
    }
}

void SystemOverviewWidget::exportTopologyAsImage(const QString& filePath)
{
    // 拓扑图导出功能（如果需要），此处仅占位
    QImage image(m_scene->sceneRect().size().toSize(), QImage::Format_ARGB32);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    m_scene->render(&painter);
    image.save(filePath);
}

void SystemOverviewWidget::setupTopologyControls()
{
    if (!m_topologyView) return;

    // 设置拓扑图的交互控制
    m_topologyView->setDragMode(QGraphicsView::ScrollHandDrag);
    m_topologyView->setRenderHint(QPainter::Antialiasing);
    m_topologyView->setRenderHint(QPainter::SmoothPixmapTransform);
    m_topologyView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_topologyView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_topologyView->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    m_topologyView->setResizeAnchor(QGraphicsView::AnchorUnderMouse);
}

void SystemOverviewWidget::parseSensorsOutput(const QString& output)
{
    QStringList lines = output.split('\n');
    bool hasTemp = false, hasFan = false, hasVolt = false;

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();

        // 温度检测
        if (trimmed.contains("°C") && (trimmed.contains("+") || trimmed.contains("-"))) {
            QRegularExpression tempRe("\\+?([0-9.]+)°C");
            QRegularExpressionMatch match = tempRe.match(trimmed);
            if (match.hasMatch()) {
                double temp = match.captured(1).toDouble();
                QString tempName = trimmed.split(":").value(0).trimmed();

                if (!hasTemp) {
                    m_textSensor->append("<b>===== 🌡️ Temperature Sensors =====</b>");
                    hasTemp = true;
                }

                QString tempColor = (temp < 50) ? "🟢" : ((temp < 80) ? "🟡" : "🔴");
                m_textSensor->append(QString("  %1 %2 : %3 °C").arg(tempColor).arg(tempName).arg(temp, 0, 'f', 1));
            }
        }

        // 风扇转速检测
        if (trimmed.contains("RPM") && trimmed.contains("+")) {
            QRegularExpression fanRe("\\+([0-9]+) RPM");
            QRegularExpressionMatch match = fanRe.match(trimmed);
            if (match.hasMatch()) {
                int rpm = match.captured(1).toInt();
                QString fanName = trimmed.split(":").value(0).trimmed();

                if (!hasFan) {
                    m_textSensor->append("<b>===== 💨 Fan Speeds =====</b>");
                    hasFan = true;
                }

                QString fanColor = (rpm < 500) ? "🟡" : ((rpm < 2000) ? "🟢" : "🔵");
                m_textSensor->append(QString("  %1 %2 : %3 RPM").arg(fanColor).arg(fanName).arg(rpm));
            }
        }

        // 电压检测
        if (trimmed.contains("V") && trimmed.contains("+") && !trimmed.contains("°C")) {
            QRegularExpression voltRe("\\+([0-9.]+) V");
            QRegularExpressionMatch match = voltRe.match(trimmed);
            if (match.hasMatch()) {
                double volt = match.captured(1).toDouble();
                QString voltName = trimmed.split(":").value(0).trimmed();

                if (!hasVolt) {
                    m_textSensor->append("<b>===== ⚡ Voltage Sensors =====</b>");
                    hasVolt = true;
                }

                m_textSensor->append(QString("  %1 : %2 V").arg(voltName).arg(volt, 0, 'f', 2));
            }
        }
    }
}

void SystemOverviewWidget::readHwmonTemperatures()
{
    QDir hwmonDir("/sys/class/hwmon");
    bool hasTemp = false;

    for (const QString& hwmon : hwmonDir.entryList()) {
        QDir deviceDir("/sys/class/hwmon/" + hwmon);

        QString deviceName = hwmon;
        QFile nameFile(deviceDir.absoluteFilePath("name"));
        if (nameFile.open(QIODevice::ReadOnly)) {
            deviceName = QString::fromUtf8(nameFile.readAll()).trimmed();
            nameFile.close();
        }

        for (const QString& file : deviceDir.entryList()) {
            if (file.startsWith("temp") && file.endsWith("_input")) {
                QFile tempFile(deviceDir.absoluteFilePath(file));
                if (tempFile.open(QIODevice::ReadOnly)) {
                    double val = tempFile.readAll().trimmed().toDouble() / 1000.0;
                    tempFile.close();

                    if (val > 0 && val < 120) {
                        QString label = file;
                        QString labelFile = file;
                        labelFile.replace("_input", "_label");
                        QFile labelF(deviceDir.absoluteFilePath(labelFile));
                        if (labelF.open(QIODevice::ReadOnly)) {
                            label = QString::fromUtf8(labelF.readAll()).trimmed();
                            labelF.close();
                        }

                        if (!hasTemp) {
                            m_textSensor->append("<b>===== 🌡️ Temperature Sensors =====</b>");
                            hasTemp = true;
                        }

                        QString tempColor = (val < 50) ? "🟢" : ((val < 80) ? "🟡" : "🔴");
                        m_textSensor->append(QString("  %1 %2 [%3] : %4 °C")
                                            .arg(tempColor).arg(label).arg(deviceName).arg(val, 0, 'f', 1));
                    }
                }
            }
        }
    }

    if (!hasTemp) {
        m_textSensor->append("  No temperature sensors detected");
    }
}

void SystemOverviewWidget::readHwmonFans()
{
    QDir hwmonDir("/sys/class/hwmon");
    bool hasFan = false;

    for (const QString& hwmon : hwmonDir.entryList()) {
        QDir deviceDir("/sys/class/hwmon/" + hwmon);

        QString deviceName = hwmon;
        QFile nameFile(deviceDir.absoluteFilePath("name"));
        if (nameFile.open(QIODevice::ReadOnly)) {
            deviceName = QString::fromUtf8(nameFile.readAll()).trimmed();
            nameFile.close();
        }

        for (const QString& file : deviceDir.entryList()) {
            if (file.startsWith("fan") && file.endsWith("_input")) {
                QFile fanFile(deviceDir.absoluteFilePath(file));
                if (fanFile.open(QIODevice::ReadOnly)) {
                    int rpm = fanFile.readAll().trimmed().toInt();
                    fanFile.close();

                    if (rpm > 0) {
                        QString label = file;
                        QString labelFile = file;
                        labelFile.replace("_input", "_label");
                        QFile labelF(deviceDir.absoluteFilePath(labelFile));
                        if (labelF.open(QIODevice::ReadOnly)) {
                            label = QString::fromUtf8(labelF.readAll()).trimmed();
                            labelF.close();
                        }

                        if (!hasFan) {
                            m_textSensor->append("<b>===== 💨 Fan Speeds =====</b>");
                            hasFan = true;
                        }

                        QString fanColor = (rpm < 500) ? "🟡" : ((rpm < 2000) ? "🟢" : "🔵");
                        m_textSensor->append(QString("  %1 %2 [%3] : %4 RPM")
                                            .arg(fanColor).arg(label).arg(deviceName).arg(rpm));
                    }
                }
            }
        }
    }

    if (!hasFan) {
        m_textSensor->append("  No fan speed sensors detected");
    }
}

void SystemOverviewWidget::readHwmonVoltages()
{
    QDir hwmonDir("/sys/class/hwmon");
    bool hasVolt = false;

    for (const QString& hwmon : hwmonDir.entryList()) {
        QDir deviceDir("/sys/class/hwmon/" + hwmon);

        QString deviceName = hwmon;
        QFile nameFile(deviceDir.absoluteFilePath("name"));
        if (nameFile.open(QIODevice::ReadOnly)) {
            deviceName = QString::fromUtf8(nameFile.readAll()).trimmed();
            nameFile.close();
        }

        for (const QString& file : deviceDir.entryList()) {
            if (file.startsWith("in") && file.endsWith("_input")) {
                QFile voltFile(deviceDir.absoluteFilePath(file));
                if (voltFile.open(QIODevice::ReadOnly)) {
                    double val = voltFile.readAll().trimmed().toDouble();
                    voltFile.close();

                    // 转换为伏特
                    if (val > 1000) {
                        val = val / 1000.0;
                    }
                    val = val / 1000.0;

                    if (val > 0 && val < 20) {
                        QString label = file;
                        QString labelFile = file;
                        labelFile.replace("_input", "_label");
                        QFile labelF(deviceDir.absoluteFilePath(labelFile));
                        if (labelF.open(QIODevice::ReadOnly)) {
                            label = QString::fromUtf8(labelF.readAll()).trimmed();
                            labelF.close();
                        }

                        if (!hasVolt) {
                            m_textSensor->append("<b>===== ⚡ Voltage Sensors =====</b>");
                            hasVolt = true;
                        }

                        m_textSensor->append(QString("  %1 [%2] : %3 V")
                                            .arg(label).arg(deviceName).arg(val, 0, 'f', 2));
                    }
                }
            }
        }
    }

    if (!hasVolt) {
        m_textSensor->append("  No voltage sensors detected");
    }
}

void SystemOverviewWidget::readHwmonSensors()
{
    readHwmonTemperatures();
    readHwmonFans();
    readHwmonVoltages();
}
