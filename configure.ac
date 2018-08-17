AC_INIT([xplugd],[1.4-beta1],[https://github.com/troglobit/xplugd/issues])
AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])
AM_SILENT_RULES([yes])

AC_CONFIG_SRCDIR([src/xplugd.c])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([Makefile man/Makefile src/Makefile])

AC_PROG_CC
AC_HEADER_STDC
AC_PROG_INSTALL

# Check for required libraries
AC_SEARCH_LIBS([pow], [m])
PKG_CHECK_MODULES([X11], [x11])
PKG_CHECK_MODULES([Xrandr], [xrandr])
PKG_CHECK_MODULES([Xi], [xi])

AC_OUTPUT
