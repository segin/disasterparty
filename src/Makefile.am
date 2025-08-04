# src/Makefile.am

lib_LTLIBRARIES = libdisasterparty.la 

libdisasterparty_la_SOURCES = disasterparty.c dp_constants.c dp_utils.c dp_context.c dp_request.c dp_message.c dp_stream.c dp_serialize.c dp_file.c dp_models.c disasterparty.h dp_private.h 

libdisasterparty_la_LDFLAGS = -version-info $(DP_LT_VERSION)
libdisasterparty_la_LIBADD = $(CURL_LIBS) $(CJSON_LIBS) 

# Use PACKAGE_TARNAME for the directory, which is 'disasterparty' (no space)
# This comes from the 4th argument to AC_INIT in configure.ac
pkgincludedir = $(includedir)/$(PACKAGE_TARNAME)
pkginclude_HEADERS = disasterparty.h 

AM_CPPFLAGS = $(CURL_CFLAGS) $(CJSON_CFLAGS) -I$(top_srcdir)/src -I$(top_srcdir)/thirdparty/libcjson/install/include -I$(top_srcdir)/thirdparty/libcjson/install/include 

