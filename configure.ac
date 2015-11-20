#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([awin], [2.00])
AM_INIT_AUTOMAKE([foreign 1.14.1])
AC_CONFIG_SRCDIR([awin.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lX11':
AC_CHECK_LIB([X11], [main])
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m], [main])

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([fcntl.h locale.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset regcomp setlocale])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
