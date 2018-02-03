QT += core
CONFIG += c++11
DEFINES *= QT_USE_QSTRINGBUILDER

INCLUDEPATH += $$PWD

OTHER_FILES += $$PWD/README.md $$PWD/mconfig.doxyfile

HEADERS += $$PWD/mconfig.h

SOURCES += $$PWD/mconfig.cpp

DISTFILES += \
    $$PWD/AUTHORS.md

DEFINES += MCONFIG_LIB
