QT       += core gui sql network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ../NotesServer/DataBase.cpp \
    ../NotesServer/NetConstants.cpp \
    ../include/AdditionalTray.cpp \
    ../include/PlatformDependent.cpp \
    FastActions.cpp \
    NetClient.cpp \
    Note.cpp \
    WidgetAlarms.cpp \
    WidgetMain.cpp \
    WidgetNoteEditor.cpp \
    main.cpp

HEADERS += \
    ../NotesServer/DataBase.h \
    ../NotesServer/NetConstants.h \
    ../include/AdditionalTray.h \
    ../include/ClickableQWidget.h \
    ../include/InputBlocker.h \
    ../include/MyQTextEdit.h \
    ../include/PlatformDependent.h \
    FastActions.h \
    NetClient.h \
    Note.h \
    Resources.h \
    WidgetAlarms.h \
    WidgetMain.h \
    WidgetNoteEditor.h

INCLUDEPATH += \
    ../include \
	../NotesServer

DEPENDPATH += \
    ../include

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
