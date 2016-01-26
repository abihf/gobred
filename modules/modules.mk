
moduledir = ${GOBRED_MODULE_DIR}

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir) $(LIBGOBRED_CFLAGS)
AM_CFLAGS = \
	-Wall \
	-std=gnu99 \
	-g
AM_LDFLAGS = -module  -avoid-version
LIBADD = $(LIBGOBRED_LIBS)

EXTRA_DIST = $(module_DATA)

