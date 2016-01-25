## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"gobred\" \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(LIBGOBRED_CFLAGS) \
	$(JSCORE_CFLAGS) \
	-I$(top_srcdir) 

AM_CFLAGS =\
	-Wall \
	-std=gnu99 \
	-g


lib_LTLIBRARIES = libgobred.la

libgobred_la_SOURCES = \
	gobred-bridge.c \
	gobred-event.c \
	gobred-js-utils.c \
	gobred-log.c \
	gobred-module.c \
	gobred-method.c \
	gobred-value.c
libgobred_la_LDFLAGS = -rdynamic -version-info 1:0
libgobred_la_LIBADD = $(LIBGOBRED_LIBS)

gobredheaders_HEADERS = \
	gobred.h \
	gobred-event.h \
	gobred-js-utils.h \
	gobred-log.h \
	gobred-method.h \
	gobred-module.h \
	gobred-value.h
gobredheadersdir = $(pkgincludedir)/libgobred

-include $(top_srcdir)/git.mk
