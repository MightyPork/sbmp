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
    sbmp/sbmp_common.c \
    sbmp/sbmp_session.c \
    main_frm_dg.c

HEADERS += \
    crc32.h \
    sbmp/sbmp.h \
    sbmp/sbmp_frame.h \
    sbmp/sbmp_datagram.h \
    sbmp/sbmp_common.h \
    sbmp/sbmp_session.h \
    sbmp/crc32.h

DISTFILES += \
    Makefile \
    style.astylerc
