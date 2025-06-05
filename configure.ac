AC_PREREQ([2.69])
AC_INIT([Disaster Party], [0.2.0], [segin2005@gmail.com], [disasterparty], [https://github.com/segin/disasterparty]) # Updated version and URL
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

# Checks for libraries.
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
# Version 0.2.0 (from 0.1.1):
# - Interface added (dp_perform_anthropic_streaming_completion, new callback type/struct) -> current increments, revision resets.
# - Age increments because interfaces were added since last public release (0.1.0 was effectively the base for 0.1.1 ABI-wise).
# current=1, revision=0, age=1 -> libdisasterparty.so.0.0.0 (soname libdisasterparty.so.0)
# This seems off. Let's use a simpler scheme for now, focusing on major.
# If Major is 0, soname is libname.so.0.
# current = major version + age.
# Let's stick to current = 0, revision = new minor, age = 0 for now for 0.x.y releases
# For 0.2.0 (from 0.1.1): current=0 (unchanged), revision=2 (minor changed), age=0 (no ABI break from a 0.x perspective)
# This would yield libdisasterparty.so.0.2.0, soname libdisasterparty.so.0
DP_LT_VERSION="0:2:0" # Updated libtool version (current:revision:age) -> current: (minor+patch) : age
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
  Library version (libtool):    ${DP_LT_VERSION} (libdisasterparty.so.0.2.0)
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
