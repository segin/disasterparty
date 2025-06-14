AC_PREREQ([2.69])
AC_INIT([Disaster Party], [0.3.0], [segin2005@gmail.com], [disasterparty], [https://github.com/segin/disasterparty])
AC_CONFIG_SRCDIR([src/disasterparty.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([m4])

AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CC
AC_PROG_LN_S
AM_PROG_AR
LT_INIT
AC_PROG_SED
AC_PROG_INSTALL
PKG_PROG_PKG_CONFIG

# Checks for libraries using pkg-config.
PKG_CHECK_MODULES([CURL], [libcurl >= 7.20.0], [],
                  [AC_MSG_ERROR([libcurl >= 7.20.0 not found. Please install libcurl-devel or equivalent.])])
AC_SUBST(CURL_CFLAGS)
AC_SUBST(CURL_LIBS)

PKG_CHECK_MODULES([CJSON], [libcjson >= 1.7.10], [],
                  [AC_CHECK_LIB([cjson], [cJSON_Parse], [],
                                [AC_MSG_ERROR([libcjson >= 1.7.10 not found. Please install libcjson-devel or equivalent.])],
                                [])])
AC_SUBST(CJSON_CFLAGS)
AC_SUBST(CJSON_LIBS)

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h stdio.h curl/curl.h cjson/cJSON.h stdbool.h stddef.h])

# Define library version for libtool
DP_LT_VERSION="1:0:0"
AC_SUBST(DP_LT_VERSION)
AC_SUBST(PACKAGE_VERSION)

AC_CONFIG_FILES([
    Makefile
    src/Makefile
    tests/Makefile
    man/Makefile
    disasterparty.pc
])
AC_OUTPUT

AC_MSG_NOTICE([
========================================================================
Disaster Party $VERSION configured successfully.

  Man pages will be installed in: ${mandir}/man3
  Library version (libtool):    ${DP_LT_VERSION} (libdisasterparty.so.1.0.0)
  Package version:              ${PACKAGE_VERSION}

  Prefix:           ${prefix}
  Compiler:         ${CC}
  CFLAGS:           ${CFLAGS}
  LIBS:             ${LIBS}
  cURL CFLAGS:      ${CURL_CFLAGS}
  cURL LIBS:        ${CURL_LIBS}
  cJSON CFLAGS:     ${CJSON_CFLAGS}
  cJSON LIBS:       ${CJSON_LIBS}

Now type 'make' to build, and then (optionally) 'make install'.
========================================================================
])
