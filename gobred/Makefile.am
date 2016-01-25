## Process this file with automake to produce Makefile.in

## Created by Anjuta



AM_CPPFLAGS = \
	-DG_LOG_DOMAIN=\"gobred\" \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	-DPACKAGE_LIB_DIR=\""$(pkglibdir)"\" \
	-I$(top_srcdir)

AM_CFLAGS = \
	-Wall \
	-std=gnu99 \
	-g

if GOBRED_PORT_GTK
bin_PROGRAMS = gobred-gtk
gobred_gtk_SOURCES = main-gtk.c
gobred_gtk_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(WEBKIT_CFLAGS) 
	
#gobred_LDFLAGS = -Wl,--export-dynamic
gobred_gtk_LDADD = $(WEBKIT_LIBS)

webkitextdir = $(pkglibdir)/webkit
webkitext_LTLIBRARIES =  GobredWebkitExtension.la
GobredWebkitExtension_la_SOURCES = extension-gtk.c
GobredWebkitExtension_la_CPPFLAGS = $(AM_CPPFLAGS) \
				$(WEBKIT_EXTENSION_CFLAGS) \
				$(JSCORE_CFLAGS) \
				$(LIBGOBRED_CFLAGS)
GobredWebkitExtension_la_LDFLAGS = -module -avoid-version
GobredWebkitExtension_la_LIBADD = $(WEBKIT_EXTENSION_LIBS) \
				$(JSCORE_LIBS) \
				../libgobred/libgobred.la
endif

if GOBRED_PORT_WPE
bin_PROGRAMS = gobred-wpe
gobred_wpe_SOURCES = main-wpe.c
gobred_wpe_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	$(WEBKIT_CFLAGS)
gobred_wpe_LDADD = $(WEBKIT_LIBS)

webkitextdir = $(pkglibdir)/webkit
webkitext_LTLIBRARIES =  GobredWebkitExtension.la
GobredWebkitExtension_la_SOURCES = extension-wpe.c
GobredWebkitExtension_la_CPPFLAGS = $(AM_CPPFLAGS) \
				$(WEBKIT_EXTENSION_CFLAGS) \
				$(JSCORE_CFLAGS) \
				$(LIBGOBRED_CFLAGS)
GobredWebkitExtension_la_LDFLAGS = -module -avoid-version -rdynamic
GobredWebkitExtension_la_LIBADD = $(WEBKIT_EXTENSION_LIBS) \
				$(JSCORE_LIBS) \
				../libgobred/libgobred.la
endif

-include $(top_srcdir)/git.mk
