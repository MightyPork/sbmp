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
    sbmp/sbmp_session.c \
    main_frm_dg.c \
    sbmp/sbmp_logging.c \
    sbmp/sbmp_checksum.c \
    sbmp/sbmp_bulk.c

HEADERS += \
    crc32.h \
    sbmp/sbmp.h \
    sbmp/sbmp_frame.h \
    sbmp/sbmp_datagram.h \
    sbmp/sbmp_session.h \
    sbmp/crc32.h \
    sbmp/sbmp_logging.h \
    sbmp/sbmp_checksum.h \
    sbmp/sbmp_config.h \
    sbmp/sbmp_bulk.h

DISTFILES += \
    Makefile \
    style.astylerc
