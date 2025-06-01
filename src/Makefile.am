# We are building a library
lib_LTLIBRARIES = libdisasterparty.la # Corrected library name

# Sources for the library
libdisasterparty_la_SOURCES = diasterparty.c diasterparty.h # Corrected filenames

# Libtool versioning
libdisasterparty_la_LDFLAGS = -version-info $(DP_LT_VERSION) $(CURL_LIBS) $(CJSON_LIBS)

# Headers to install
# pkgincludedir is $(includedir)/<packagename> as defined in configure.ac
pkgincludedir = $(includedir)/disasterparty # Corrected directory name
pkginclude_HEADERS = diasterparty.h # Corrected header name

# AM_CPPFLAGS can be used to add include paths
AM_CPPFLAGS = $(CURL_CFLAGS) $(CJSON_CFLAGS) -I$(top_srcdir)/src

