TEMPLATE = app
TARGET = ClipboardSync
CONFIG += c++17 
QT += core gui widgets network
DEFINES += AES256=1 ECB=0 CTR=0

HEADERS += \
    ClipboardManager.h \
    ConfigDialog.h \
    ConfigManager.h \
    CryptoEngine.h \
    DatagramProcessor.h

SOURCES += \
    main.cpp \
    ClipboardManager.cpp \
    ConfigDialog.cpp \
    ConfigManager.cpp \
    CryptoEngine.cpp \
    DatagramProcessor.cpp\
    thirdparty\tiny-AES-c\aes.c

RESOURCES += resources.qrc


win32:CONFIG += windows suppress_vcproj_warnings
# macx:QMAKE_INFO_PLIST = Info.plist
