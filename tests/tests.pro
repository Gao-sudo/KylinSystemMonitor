QT += core testlib concurrent
CONFIG += testcase console c++17
TEMPLATE = app
TARGET = kylin_service_tests

INCLUDEPATH += \
    $$PWD/.. \
    $$PWD/../service

SOURCES += \
    $$PWD/test_service.cpp \
    $$PWD/../service/cryptohelper.cpp \
    $$PWD/../service/asyncmonitor.cpp

HEADERS += \
    $$PWD/../service/cryptohelper.h \
    $$PWD/../service/asyncmonitor.h

QMAKE_CXXFLAGS += --coverage -O0
QMAKE_LFLAGS += --coverage

LIBS += -lssl -lcrypto -pthread
