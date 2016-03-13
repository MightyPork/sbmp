TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DEFINES =

INCLUDEPATH =

SOURCES += \
    main.c \
    sbmp/crc32.c \
    sbmp/sbmp_frame.c \
    sbmp/sbmp_datagram.c \
    sbmp/sbmp_common.c

HEADERS += \
    crc32.h \
    sbmp/sbmp.h \
    sbmp/sbmp_frame.h \
    sbmp/sbmp_datagram.h \
    sbmp/sbmp_frame_internal.h \
    sbmp/sbmp_common.h

DISTFILES += \
    Makefile \
    style.astylerc
