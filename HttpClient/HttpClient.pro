QT       += core gui widgets network

CONFIG += c++17

SOURCES += \
    HttpClient.cpp \
    main.cpp \
    WindowClient.cpp

HEADERS += \
    HttpClient.h \
    WindowClient.h

INCLUDEPATH += \
    C:/Qt/boost_1_72_0 \
	../../include

DEPENDPATH += \
    C:/Qt/boost_1_72_0 \
	../../include

LIBS += -lws2_32

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
