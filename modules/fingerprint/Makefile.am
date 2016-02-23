if BUILD_FINGERPRINT_MODULE

module_LTLIBRARIES = GobredFingerPrint.la

GobredFingerPrint_la_SOURCES = \
	fingerprint.c \
	poll.c \
	enroll.c \
	util.c

# module_DATA = hello.module
GobredFingerPrint_la_CPPFLAGS = $(AM_CPPFLAGS) $(LIBFPRINT_CFLAGS)
GobredFingerPrint_la_LIBADD = $(LIBADD) $(LIBFPRINT_LIBS)


include ../modules.mk


-include $(top_srcdir)/git.mk

endif
