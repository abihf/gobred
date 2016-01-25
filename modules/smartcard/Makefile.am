if BUILD_SMARTCARD_MODULE

module_LTLIBRARIES = GobredSmartCard.la

GobredSmartCard_la_SOURCES = \
	smartcard.c

# module_DATA = hello.module
GobredSmartCard_la_CPPFLAGS = $(AM_CPPFLAGS) $(LIBPCSCLITE_CFLAGS)
GobredSmartCard_la_LIBADD = $(LIBADD) $(LIBPCSCLITE_LIBS)


include ../modules.mk


-include $(top_srcdir)/git.mk

endif
