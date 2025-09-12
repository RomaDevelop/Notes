QT       += core gui sql network widgets

CONFIG += c++17

SOURCES += \
    ../CommitsParser/git.cpp \
    ../NotesServer/DataBase.cpp \
    ../NotesServer/NetConstants.cpp \
    ../include/AdditionalTray.cpp \
    ../include/PlatformDependent.cpp \
    DialogInputTime.cpp \
    FastActions.cpp \
    NetClient.cpp \
    Note.cpp \
    WidgetAlarms.cpp \
    WidgetMain.cpp \
    WidgetNoteEditor.cpp \
    main.cpp

HEADERS += \
    ../CommitsParser/git.h \
    ../NotesServer/DataBase.h \
    ../NotesServer/NetConstants.h \
    ../include/AdditionalTray.h \
    ../include/ClickableQWidget.h \
    ../include/InputBlocker.h \
    ../include/MyQTextEdit.h \
    ../include/PlatformDependent.h \
    ../include/TextEditCleaner.h \
    DevNames.h \
    DialogInputTime.h \
    FastActions.h \
    NetClient.h \
    Note.h \
    Resources.h \
    WidgetAlarms.h \
    WidgetMain.h \
    WidgetNoteEditor.h

INCLUDEPATH += \
    ../include \
	../NotesServer \
	../CommitsParser

DEPENDPATH += \
    ../include

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
