lib_LTLIBRARIES = libdisasterparty.la # Corrected library name

libdisasterparty_la_SOURCES = disasterparty.c disasterparty.h # Corrected filenames

libdisasterparty_la_LDFLAGS = -version-info $(DP_LT_VERSION)
libdisasterparty_la_LIBADD = $(CURL_LIBS) $(CJSON_LIBS) # Added CJSON_LIBS

pkgincludedir = $(includedir)/disasterparty # Corrected directory name
pkginclude_HEADERS = disasterparty.h # Corrected header name

AM_CPPFLAGS = $(CURL_CFLAGS) $(CJSON_CFLAGS) -I$(top_srcdir)/src 

