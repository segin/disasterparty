## src/Makefile.am
lib_LTLIBRARIES = libdisasterparty.la
libdisasterparty_la_SOURCES = dp_context.c dp_request.c dp_message.c dp_serialize.c dp_file.c dp_constants.c dp_utils.c dp_stream.c dp_models.c
AM_CPPFLAGS = $(CURL_CFLAGS) $(CJSON_CFLAGS) -I$(top_srcdir)/src
pkgincludedir = $(includedir)/$(PACKAGE_TARNAME)
pkginclude_HEADERS = disasterparty.h
libdisasterparty_la_LDFLAGS = -version-info $(DP_LT_VERSION)
libdisasterparty_la_LIBADD = $(CURL_LIBS) $(CJSON_LIBS)
