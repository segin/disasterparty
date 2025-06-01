AC_PREREQ([2.69])
AC_INIT([Disaster Party], [0.1.0], [your-email@example.com], [disasterparty], [https://example.com/disasterparty])
AC_CONFIG_SRCDIR([src/diasterparty.c]) # Corrected filename
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([m4])

AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CC
AC_PROG_LN_S
LT_INIT # Initialize libtool

# Checks for libraries.
# Check for libcurl
PKG_CHECK_MODULES([CURL], [libcurl >= 7.20.0], [],
                  [AC_MSG_ERROR([libcurl >= 7.20.0 not found. Please install libcurl-devel or equivalent.])])
AC_SUBST(CURL_CFLAGS)
AC_SUBST(CURL_LIBS)

# Check for libcjson
PKG_CHECK_MODULES([CJSON], [libcjson >= 1.7.10], [], # Check common pkg-config name for cJSON
                  [AC_CHECK_LIB([cjson], [cJSON_Parse], [], # Fallback to direct lib check
                                [AC_MSG_ERROR([libcjson not found. Please install libcjson-devel or equivalent.])],
                                [])]) # Empty action-if-not-found for AC_CHECK_LIB if PKG_CHECK_MODULES fails but this succeeds.
AC_SUBST(CJSON_CFLAGS)
AC_SUBST(CJSON_LIBS)


# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h stdio.h curl/curl.h cjson/cJSON.h stdbool.h stddef.h])

# Define library version for libtool
# Version 0.1.0 -> current=0, revision=1, age=0 -> libdisasterparty.so.0.1.0
# Soname: libdisasterparty.so.0 (current - age)
DP_LT_VERSION="0:1:0" # Major version 0, first revision of this API (minor 1), no prior ABI (patch 0)
AC_SUBST(DP_LT_VERSION)


AC_CONFIG_FILES([
    Makefile
    src/Makefile
    tests/Makefile
    disasterparty.pc
])
AC_OUTPUT

AC_MSG_NOTICE([
========================================================================
Disaster Party $VERSION configured successfully.

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

