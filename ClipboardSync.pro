TEMPLATE = app
TARGET = ClipboardSync
CONFIG += c++17 
QT += core gui widgets network

HEADERS += \
    ConfigDialog.h \
    ConfigManager.h \
    ClipboardManager.h \
    DatagramProcessor.h

SOURCES += \
    main.cpp \
    ConfigDialog.cpp \
    ConfigManager.cpp \
    ClipboardManager.cpp \
    DatagramProcessor.cpp

RESOURCES += resources.qrc


win32:CONFIG += windows suppress_vcproj_warnings
# macx:QMAKE_INFO_PLIST = Info.plist
