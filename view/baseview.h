#ifndef BASEVIEW_H
#define BASEVIEW_H

#include <QWidget>

class BaseView : public QWidget
{
    Q_OBJECT

public:
    explicit BaseView(QWidget *parent = nullptr) : QWidget(parent) {}
    virtual ~BaseView() {}

    virtual void updateView() = 0;  // 子类必须实现

signals:
    void actionRequested(const QString& action, const QVariant& data);
};

#endif
