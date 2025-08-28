QT       += core gui sql core5compat network widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
RC_ICONS = images/QQ.ico
SOURCES += \
    enrollstatewidget.cpp \
    ChatDelegate.cpp \
    enrollwindow.cpp \
    enrollworker.cpp \
    load.cpp \
    localmessagemanager.cpp \
    loginstatewidget.cpp \
    loginworker.cpp \
    logoutworker.cpp \
    main.cpp \
    messageitemwidget.cpp \
    updatemanager.cpp \
    usermanager.cpp \
    networkmanager.cpp \
    widget.cpp

HEADERS += \
    enrollstatewidget.h \
    enrollwindow.h \
    enrollworker.h \
    load.h \
    localmessagemanager.h \
    loginstatewidget.h \
    loginworker.h \
    logoutworker.h \
    messageitemwidget.h \
    updatemanager.h \
    usermanager.h \
    ChatDelegate.h \
    networkmanager.h \
    widget.h

FORMS += \
    enrollstatewidget.ui \
    enrollwindow.ui \
    load.ui \
    loginstatewidget.ui \
    widget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
win32:RC_ICONS = images/QQ.ico

RESOURCES += \
    src.qrc
