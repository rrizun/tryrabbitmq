AC_INIT(tryrabbitmq, 0.9.0)
AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror -Wno-portability])

#AC_PROG_CC
#AM_PROG_CC_C_O
AC_PROG_CXX
#AC_PROG_RANLIB
#AC_PROG_LIBTOOL

AC_CONFIG_FILES([Makefile])

AC_CHECK_LIB([dl], [main])
AC_CHECK_LIB([rt], [main])
AC_CHECK_LIB([pthread], [main])
#AC_CHECK_LIB([mysql], [main])

PKG_CHECK_MODULES([MODULES], [libcurl libidn openssl protobuf libxml-2.0])

AC_OUTPUT
