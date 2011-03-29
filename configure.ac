AC_INIT([usf],[1.0.0], [andreas.sandberg@it.uu.se])
AC_PREREQ(2.59)
AC_CONFIG_AUX_DIR([.])
AM_INIT_AUTOMAKE([foreign])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

AC_PROG_CC_STDC

AC_CHECK_HEADERS([argp.h], 
  [ have_argp=true ], [
    AC_MSG_WARN([GNU Argp not found, some tools will be disabled.])
    have_argp=false
  ])
AM_CONDITIONAL([HAVE_ARGP], [ test x$have_argp = xtrue ])

AC_CHECK_FUNCS([strndup strnlen])
AC_CHECK_LIB([bz2], [BZ2_bzReadOpen])

AC_ARG_ENABLE([debug-log],
  AS_HELP_STRING([--enable-debug-log],
    [Enable debug logging (default: disabled)]),
  [
    AC_DEFINE([ENABLE_DEBUG_LOG], [1], [Enable debug logging.])
  ])

AC_ARG_ENABLE([strict],
  AS_HELP_STRING([--disable-strict],
    [Disable strict compile time checks.]),
  [
    true
  ], [
    CFLAGS="-Werror ${CFLAGS}"
  ])

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([
	Makefile

	include/Makefile
	include/uart/Makefile

	lib/Makefile

	tools/Makefile

	test/Makefile
])

AC_OUTPUT
