TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DEFINES =

INCLUDEPATH =

SOURCES += \
	main.c \
	crc32.c \
	sbmp.c

HEADERS += \
	crc32.h \
	sbmp.h

DISTFILES += \
	Makefile \
	style.astylerc
