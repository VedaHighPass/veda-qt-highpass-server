QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network sql

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    DatabaseManager.cpp \
    httpserver.cpp \
    main.cpp \
    mainwindow.cpp \
    rtpclient.cpp \
    videostream.cpp

HEADERS += \
    DatabaseManager.h \
    httpserver.h \
    mainwindow.h \
    rtpclient.h \
    videostream.h

FORMS += \
    mainwindow.ui \
    videostream.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc