AC_INIT([microsocks],[1.0.1])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_AUX_DIR([config])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Pthread Support
AX_PTHREAD
LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"

# Systemd Check
PKG_PROG_PKG_CONFIG
AC_ARG_WITH([systemdsystemunitdir],
        AS_HELP_STRING([--with-systemdsystemunitdir=DIR], [Directory for systemd service files]),
        [], [with_systemdsystemunitdir=$($PKG_CONFIG --variable=systemdsystemunitdir systemd)])
AC_SUBST([systemdsystemunitdir], [$with_systemdsystemunitdir])
AM_CONDITIONAL(HAVE_SYSTEMD, [test -n "$with_systemdsystemunitdir"])

AC_CONFIG_FILES([Makefile
src/Makefile
systemd/Makefile
systemd/microsocks.service])

AC_OUTPUT
