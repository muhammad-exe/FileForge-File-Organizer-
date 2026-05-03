QT += widgets

TEMPLATE = app
TARGET   = FileForge
CONFIG  += c++17
CONFIG  += warn_on

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h \
    fileorganizer.h \
    organizerworker.h
    RC_ICONS = fileforge.ico