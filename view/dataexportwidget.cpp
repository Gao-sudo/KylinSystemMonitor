// view/dataexportwidget.cpp
#include "dataexportwidget.h"
#include "../controller/exportcontroller.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDir>

DataExportWidget::DataExportWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
}

void DataExportWidget::initUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // 创建按钮
    btnExportJson = new QPushButton("📄 Export System Data to JSON");
    btnExportCsv  = new QPushButton("📊 Export System Data to CSV");
    btnExportEncrypted = new QPushButton("🔐 Export Encrypted Data (SM4)");

    btnExportProcessJson = new QPushButton("📋 Export Process Data to JSON");
    btnExportProcessCsv  = new QPushButton("📁 Export Process Data to CSV");

    // 设置按钮大小
    btnExportJson->setFixedSize(280, 45);
    btnExportCsv->setFixedSize(280, 45);
    btnExportEncrypted->setFixedSize(280, 45);
    btnExportProcessJson->setFixedSize(280, 45);
    btnExportProcessCsv->setFixedSize(280, 45);

    // 设置样式
    QString blueStyle = "QPushButton { background: #3498db; color: white; border-radius: 5px; font-size: 12px; }"
                        "QPushButton:hover { background: #2980b9; }";
    QString greenStyle = "QPushButton { background: #27ae60; color: white; border-radius: 5px; font-size: 12px; }"
                         "QPushButton:hover { background: #229954; }";
    QString purpleStyle = "QPushButton { background: #9b59b6; color: white; border-radius: 5px; font-size: 12px; }"
                          "QPushButton:hover { background: #8e44ad; }";

    btnExportJson->setStyleSheet(blueStyle);
    btnExportCsv->setStyleSheet(greenStyle);
    btnExportEncrypted->setStyleSheet(purpleStyle);
    btnExportProcessJson->setStyleSheet(blueStyle);
    btnExportProcessCsv->setStyleSheet(greenStyle);

    // 添加按钮到布局
    layout->addStretch();
    layout->addWidget(btnExportJson, 0, Qt::AlignCenter);
    layout->addSpacing(15);
    layout->addWidget(btnExportCsv, 0, Qt::AlignCenter);
    layout->addSpacing(15);
    layout->addWidget(btnExportEncrypted, 0, Qt::AlignCenter);
    layout->addSpacing(30);
    layout->addWidget(btnExportProcessJson, 0, Qt::AlignCenter);
    layout->addSpacing(15);
    layout->addWidget(btnExportProcessCsv, 0, Qt::AlignCenter);
    layout->addStretch();

    // 连接信号槽
    connect(btnExportJson, &QPushButton::clicked, this, &DataExportWidget::exportSystemJson);
    connect(btnExportCsv, &QPushButton::clicked, this, &DataExportWidget::exportSystemCsv);
    connect(btnExportEncrypted, &QPushButton::clicked, this, &DataExportWidget::exportEncrypted);
    connect(btnExportProcessJson, &QPushButton::clicked, this, &DataExportWidget::exportProcessJson);
    connect(btnExportProcessCsv, &QPushButton::clicked, this, &DataExportWidget::exportProcessCsv);
}

void DataExportWidget::exportSystemJson()
{
    QString path = QFileDialog::getSaveFileName(this, "Export System JSON",
                                                QDir::homePath() + "/system_report.json",
                                                "JSON (*.json)");
    if (path.isEmpty()) return;

    if (ExportController::instance().exportToJson(path)) {
        QMessageBox::information(this, "Success", "JSON exported to:\n" + path);
    } else {
        QMessageBox::warning(this, "Error", "Failed to export JSON");
    }
}

void DataExportWidget::exportSystemCsv()
{
    QString path = QFileDialog::getSaveFileName(this, "Export System CSV",
                                                QDir::homePath() + "/system_report.csv",
                                                "CSV (*.csv)");
    if (path.isEmpty()) return;

    if (ExportController::instance().exportToCsv(path)) {
        QMessageBox::information(this, "Success", "CSV exported to:\n" + path);
    } else {
        QMessageBox::warning(this, "Error", "Failed to export CSV");
    }
}

void DataExportWidget::exportEncrypted()
{
    QString path = QFileDialog::getSaveFileName(this, "Export Encrypted Data",
                                                QDir::homePath() + "/encrypted_report.sm4",
                                                "SM4 Encrypted (*.sm4)");
    if (path.isEmpty()) return;

    if (ExportController::instance().exportEncryptedToJson(path)) {
        QMessageBox::information(this, "Success",
                                "Encrypted data exported to:\n" + path + "\n"
                                "Key saved to:\n" + path + ".key\n\n"
                                "Keep the key file secure!");
    } else {
        QMessageBox::warning(this, "Error", "Failed to export encrypted data");
    }
}

void DataExportWidget::exportProcessJson()
{
    bool ok;
    int pid = QInputDialog::getInt(this, "Export Process JSON",
                                   "Enter Process ID:", 1, 1, 100000, 1, &ok);
    if (!ok) return;

    QString path = QFileDialog::getSaveFileName(this, "Export Process JSON",
                                                QDir::homePath() + "/process_" + QString::number(pid) + ".json",
                                                "JSON (*.json)");
    if (path.isEmpty()) return;

    if (ExportController::instance().exportProcessToJson(pid, path)) {
        QMessageBox::information(this, "Success", "Process JSON exported to:\n" + path);
    } else {
        QMessageBox::warning(this, "Error", "Failed to export process JSON");
    }
}

void DataExportWidget::exportProcessCsv()
{
    bool ok;
    int pid = QInputDialog::getInt(this, "Export Process CSV",
                                   "Enter Process ID:", 1, 1, 100000, 1, &ok);
    if (!ok) return;

    QString path = QFileDialog::getSaveFileName(this, "Export Process CSV",
                                                QDir::homePath() + "/process_" + QString::number(pid) + ".csv",
                                                "CSV (*.csv)");
    if (path.isEmpty()) return;

    if (ExportController::instance().exportProcessToCsv(pid, path)) {
        QMessageBox::information(this, "Success", "Process CSV exported to:\n" + path);
    } else {
        QMessageBox::warning(this, "Error", "Failed to export process CSV");
    }
}
