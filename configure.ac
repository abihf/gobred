dnl Process this file with autoconf to produce a configure script.
dnl Created by Anjuta application wizard.

AC_INIT(gobred, 0.1)

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_MACRO_DIR(m4)

AM_INIT_AUTOMAKE([1.11 subdir-objects])

AM_SILENT_RULES([yes])

AC_PROG_CC




dnl ***************************************************************************
dnl Internationalization
dnl ***************************************************************************
IT_PROG_INTLTOOL([0.35.0])

GETTEXT_PACKAGE=gobred
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])

AS_AC_EXPAND(LIBDIR, $libdir)
PACKAGE_LIBDIR=$LIBDIR/gobred
AC_SUBST(PACKAGE_LIBDIR)
AM_GLIB_GNU_GETTEXT

LT_INIT


PKG_CHECK_MODULES(LIBGOBRED, [glib-2.0])


m4_define([default_port], [gtk])
AC_ARG_WITH([port],
            [AC_HELP_STRING([--with-port=@<:@gtk/wpe@:>@],
                            [Select the Webkit port [default=gtk]])],
	    [],
            [with_port=default_port])

AS_CASE([$with_port],
	[gtk], [
		webkit_pc="webkit2gtk-4.0 gtk+-3.0"
		webkit_extension_pc="webkit2gtk-web-extension-4.0"
		jscore_pc="javascriptcoregtk-4.0"

	],
	[wpe], [
		webkit_pc="wpe-webkit"
		webkit_extension_pc="wpe-webkit"
		jscore_pc="wpe-webkit"

	],

	[AC_MSG_ERROR([Invalid argument for --with-port])]
)

PKG_CHECK_MODULES(WEBKIT, $webkit_pc)
PKG_CHECK_MODULES(WEBKIT_EXTENSION, $webkit_extension_pc)
PKG_CHECK_MODULES(JSCORE, $jscore_pc)

GOBRED_PORT=$with_port
AC_SUBST(GOBRED_PORT)
AM_CONDITIONAL(GOBRED_PORT_GTK, test "x$with_port" = "xgtk")
AM_CONDITIONAL(GOBRED_PORT_WPE, test "x$with_port" = "xwpe")
AC_DEFINE_UNQUOTED(GOBRED_PORT,"$GOBRED_PORT", [webkit port])

GOBRED_MODULE_DIR="$PACKAGE_LIBDIR/modules"
AC_SUBST(GOBRED_MODULE_DIR)
AC_DEFINE_UNQUOTED(GOBRED_MODULE_DIR,"$GOBRED_MODULE_DIR", [module dir])


dnl =============================Modules=======================================

dnl ==== smartcard ====
AC_ARG_ENABLE(smartcard-module,
			  AC_HELP_STRING([--enable-smartcard-module],
					[enable the smartcard module. [default=yes]]),
			  [want_smartcard_module=$enableval],[want_smartcard_module="yes"])

if test x"$want_smartcard_module" = x"yes" ; then
	PKG_CHECK_MODULES(LIBPCSCLITE, [libpcsclite])
fi

AM_CONDITIONAL(BUILD_SMARTCARD_MODULE, test "x$want_smartcard_module" = "xyes")
#AC_SUBST(BUILD_SMARTCARD_MODULE)

dnl ==== fingerprint ====
AC_ARG_ENABLE(fingerprint-module,
			  AC_HELP_STRING([--enable-fingerprint-module],
					[enable the fingerprint module. [default=yes]]),
			  [want_fingerprint_module=$enableval],[want_fingerprint_module="yes"])

if test x"$want_fingerprint_module" = x"yes" ; then
	PKG_CHECK_MODULES(LIBFPRINT, [libfprint gdk-pixbuf-2.0])
fi

AM_CONDITIONAL(BUILD_FINGERPRINT_MODULE, test "x$want_fingerprint_module" = "xyes")
#AC_SUBST(BUILD_FINGERPRINT_MODULE)





AC_OUTPUT([
Makefile
libgobred/Makefile
gobred/Makefile
modules/Makefile
modules/fingerprint/Makefile
modules/smartcard/Makefile
po/Makefile.in
])
