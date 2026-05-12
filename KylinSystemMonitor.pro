# KylinSystemMonitor.pro
QT += core gui widgets charts sql network concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
QMAKE_CXXFLAGS += -isystem /usr/include/c++/11
QMAKE_CXXFLAGS += -isystem /usr/include/x86_64-linux-gnu/c++/11

# 添加多线程优化
QMAKE_CXXFLAGS += -O2 -pthread
LIBS += -pthread

# ==============================================
# MVC 架构头文件包含路径
# ==============================================
INCLUDEPATH += \
    $$PWD \
    $$PWD/controller \
    $$PWD/model \
    $$PWD/view

# ==============================================
# Controller 层（业务逻辑控制）
# ==============================================
SOURCES += \
    $$PWD/controller/processcontroller.cpp \
    $$PWD/controller/systemcontroller.cpp \
    $$PWD/controller/filesystemcontroller.cpp \
    $$PWD/controller/exportcontroller.cpp

HEADERS += \
    $$PWD/controller/processcontroller.h \
    $$PWD/controller/systemcontroller.h \
    $$PWD/controller/filesystemcontroller.h \
    $$PWD/controller/exportcontroller.h

# ==============================================
# Model 层（数据存储 + 读取）
# ==============================================
SOURCES += \
    $$PWD/model/processmodel.cpp \
    $$PWD/model/systemmodel.cpp \
    $$PWD/model/sensorsreader.cpp\
    $$PWD/model/sensormodel.cpp

HEADERS += \
    $$PWD/model/processmodel.h \
    $$PWD/model/systemmodel.h \
    $$PWD/model/sensorsreader.h \
    $$PWD/model/sensormodel.h

# ==============================================
# Service 层（独立服务）
# ==============================================
SOURCES += \
    $$PWD/service/asyncmonitor.cpp \
    $$PWD/service/cryptohelper.cpp

HEADERS += \
    $$PWD/service/asyncmonitor.h \
    $$PWD/service/cryptohelper.h
# ==============================================
# View 层（UI 界面）
# ==============================================
SOURCES += \
    $$PWD/view/systemoverviewwidget.cpp \
    $$PWD/view/resourcemonitorwidget.cpp \
    $$PWD/view/processmanagerwidget.cpp \
    $$PWD/view/filesystemmonitor.cpp \
    $$PWD/view/dataexportwidget.cpp \
    $$PWD/view/processreportdialog.cpp

HEADERS += \
    $$PWD/view/systemoverviewwidget.h \
    $$PWD/view/resourcemonitorwidget.h \
    $$PWD/view/processmanagerwidget.h \
    $$PWD/view/filesystemmonitor.h \
    $$PWD/view/dataexportwidget.h \
    $$PWD/view/processreportdialog.h

# ==============================================
# 主程序入口
# ==============================================
SOURCES += \
    $$PWD/main.cpp \
    $$PWD/mainwindow.cpp

HEADERS += \
    $$PWD/mainwindow.h

# ==============================================
# 翻译 & 库依赖
# ==============================================
TRANSLATIONS += $$PWD/KylinSystemMonitor_zh_CN.ts
LIBS += -lssl -lcrypto

# ==============================================
# 安装配置
# ==============================================
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    cpu_profile.sh \
    memcheck.sh \
    performance_test.sh
