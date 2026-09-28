AC_INIT([tclwhisper], [0.1])
AC_CONFIG_SRCDIR([generic/tclwhisper.c])
AC_CONFIG_AUX_DIR([tclconfig])

TEA_INIT([3.9])
TEA_PATH_TCLCONFIG
TEA_LOAD_TCLCONFIG
TEA_CONFIG_SYSTEM
TEA_ADD_SOURCES([tclwhisper.c])

AC_PROG_CC
AC_CANONICAL_HOST
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([WHISPER], [whisper])

WHISPER_LIBDIR=`$PKG_CONFIG --variable=libdir whisper`
AS_IF([test -z "$WHISPER_LIBDIR"], [
    AC_MSG_ERROR([pkg-config did not report a libdir for whisper])
])

dnl The local upstream prefix is not registered with the dynamic loader.
dnl Keep its discovered libdir available both while linking and at runtime.
case "$host_os" in
    linux*)
        WHISPER_LINK_LDFLAGS="-Wl,-rpath-link,$WHISPER_LIBDIR"
        WHISPER_RUNTIME_LDFLAGS="-Wl,--disable-new-dtags,-rpath,$WHISPER_LIBDIR"
        ;;
    darwin*)
        WHISPER_LINK_LDFLAGS=""
        WHISPER_RUNTIME_LDFLAGS="-Wl,-rpath,$WHISPER_LIBDIR"
        ;;
    *)
        WHISPER_LINK_LDFLAGS=""
        WHISPER_RUNTIME_LDFLAGS=""
        ;;
esac

save_CPPFLAGS="$CPPFLAGS"
save_LIBS="$LIBS"
save_LDFLAGS="$LDFLAGS"
CPPFLAGS="$CPPFLAGS $WHISPER_CFLAGS"
LIBS="$WHISPER_LIBS $LIBS"
LDFLAGS="$LDFLAGS $WHISPER_LINK_LDFLAGS"
AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([[#include <whisper.h>]], [[return whisper_version() == 0;]])],
    [],
    [AC_MSG_ERROR([the discovered whisper library cannot be linked])])
CPPFLAGS="$save_CPPFLAGS"
LIBS="$save_LIBS"
LDFLAGS="$save_LDFLAGS"

TEA_ENABLE_SHARED
TEA_ENABLE_THREADS
TEA_PUBLIC_TCL_HEADERS
TEA_CONFIG_CFLAGS
TEA_MAKE_LIB
TEA_PROG_TCLSH

PKG_LIB_FILE="tclwhisper${SHLIB_SUFFIX}"
AC_SUBST([PKG_LIB_FILE])
AC_SUBST([TCL_INCLUDE_SPEC])
AC_SUBST([TCL_STUB_LIB_SPEC])
AC_SUBST([WHISPER_LIBDIR])
AC_SUBST([WHISPER_LINK_LDFLAGS])
AC_SUBST([WHISPER_RUNTIME_LDFLAGS])

AC_CONFIG_FILES([Makefile pkgIndex.tcl])
AC_OUTPUT
