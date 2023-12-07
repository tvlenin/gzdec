AC_PREREQ([2.71])

AC_INIT([gzdec], [1.0.0], [ttvleninn@gmail.com])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AM_PROG_AR

LT_INIT
AC_PROG_CC

AC_ENABLE_SHARED



PKG_CHECK_MODULES([GST], [gstreamer-1.0, gstreamer-base-1.0],[],[AC_MSG_ERROR([cannot build plugin, failed to find gstreamer-1.0 (compilation requirement)])])
PKG_CHECK_MODULES([ZLIB], [zlib], [AC_DEFINE(HAVE_ZLIB, 1, [Define if zlib is available])], [AC_MSG_ERROR([cannot build plugin, failed to find zlib (compilation requirement)])])

dnl bzip2 does not provide pkg-config so it is necessary to 
dnl do the linking manually

save_LIBS=$LIBS
LIBS="$LIBS -lbz2"
AC_MSG_CHECKING([for BZ2_bzlibVersion in -lbz2])
AC_LINK_IFELSE([AC_LANG_PROGRAM([[#include <bzlib.h>]], [[const char *version = BZ2_bzlibVersion ();]])],[HAVE_BZ2=yes],[HAVE_BZ2=no])
AC_MSG_RESULT($HAVE_BZ2)
LIBS=$save_LIBS
if test "x$HAVE_BZ2" = "xyes"; then
AC_DEFINE(HAVE_BZ2,[1],[Has BZ2])
BZ2_LIBS="-lbz2"
fi
AC_SUBST(BZ2_LIBS)

AC_CONFIG_FILES([Makefile src/Makefile])

AC_DEFINE(PLUGIN_DESCRIPTION,"gzip and bzip decompresser gstreamer plugin")
AC_DEFINE(GST_PACKAGE_NAME,"gzdec gstreamer plugin")
AC_DEFINE(GST_PACKAGE_ORIGIN,"github.com/tvlenin/gzdec")
AC_DEFINE(GST_LICENSE,"LGPL")

AC_OUTPUT