AC_INIT([pdbg], [1.0])
AM_INIT_AUTOMAKE([subdir-objects])
AM_SILENT_RULES([yes])

AC_PROG_CC
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_LANG(C)

case "$host" in
	arm*-*-*)       ARCH="arm" ;;
	powerpc*-*-*)   ARCH="ppc" ;;
	*)              ARCH="def" ;;
esac
AC_SUBST([ARCH])

AC_OUTPUT
