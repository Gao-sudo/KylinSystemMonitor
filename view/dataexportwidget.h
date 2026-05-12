#ifndef DATAEXPORTWIDGET_H
#define DATAEXPORTWIDGET_H

#include <QWidget>

class QPushButton;

class DataExportWidget : public QWidget
{
    Q_OBJECT
public:
    explicit DataExportWidget(QWidget *parent = nullptr);

private slots:
    void exportSystemJson();
    void exportSystemCsv();
    void exportEncrypted();
    void exportProcessJson();
    void exportProcessCsv();

private:
    void initUI();

    QPushButton *btnExportJson;
    QPushButton *btnExportCsv;
    QPushButton *btnExportEncrypted;
    QPushButton *btnExportProcessJson;
    QPushButton *btnExportProcessCsv;
};

#endif
