# We are building a library
lib_LTLIBRARIES = libdiasterparty.la

# Sources for the library
libdiasterparty_la_SOURCES = diasterparty.c diasterparty.h

# Libtool versioning
libdiasterparty_la_LDFLAGS = -version-info $(DIASTERPARTY_LT_VERSION)

# Headers to install
# pkgincludedir is $(includedir)/<packagename> as defined in configure.ac
pkgincludedir = $(includedir)/diasterparty
pkginclude_HEADERS = diasterparty.h

# Linker flags for libcurl if needed directly by the library (usually not, but good practice)
libdiasterparty_la_LIBADD = $(CURL_LIBS)

# AM_CPPFLAGS can be used to add include paths, e.g. -I$(top_srcdir)/include
AM_CPPFLAGS = -I$(top_srcdir)/src # Ensure diasterparty.h is found if not in current dir

