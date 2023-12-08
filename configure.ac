dnl AC_PREREQ([2.71])

AC_INIT([gzdec], [1.0.0], [ttvleninn@gmail.com])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])

AM_PROG_AR

LT_INIT
AC_PROG_CC

AC_ENABLE_SHARED


# Default GStreamer version
GST_DEFAULT_VERSION=1.0

# Initialize with a default value
GST_VERSION=$GST_DEFAULT_VERSION

# Check for the --with-gst-version option
AC_ARG_WITH([gst-version],
  [AS_HELP_STRING([--with-gst-version=VERSION], [Specify GStreamer version (default is 1.0)])],
  [GST_VERSION="$withval"])
PKG_CHECK_MODULES([ZLIB], [zlib], [AC_DEFINE(HAVE_ZLIB, 1, [Define if zlib is available])], [AC_MSG_ERROR([cannot build plugin, failed to find zlib (compilation requirement)])])

# Check the selected GStreamer version
case "$GST_VERSION" in
  1.0)
    AM_CONDITIONAL(GST_VERSION_1_0, test "enable"=yes)
    AC_DEFINE(GST_VERSION_1_0, 1, Define if GStreamer version is 1.0)
    PKG_CHECK_MODULES([GST], [gstreamer-1.0, gstreamer-base-1.0],[],[AC_MSG_ERROR([cannot build plugin, failed to find gstreamer-1.0 (compilation requirement)])])


    ;;
  0.1)
    AM_CONDITIONAL(GST_VERSION_1_0, false)

    PKG_CHECK_MODULES([GST], [gstreamer-0.10, gstreamer-base-0.10],[],[AC_MSG_ERROR([cannot build plugin, failed to find gstreamer-1.0 (compilation requirement)])])
    ;;
  *)
    AC_MSG_ERROR([Unsupported GStreamer version: $GST_VERSION])
    ;;
esac


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
