# Programs to build for tests
check_PROGRAMS = \
    test_openai_text_dp \
    test_openai_multimodal_dp \
    test_gemini_text_dp \
    test_gemini_multimodal_dp \
    test_openai_streaming_dp \
    test_gemini_streaming_dp

# Sources for each test program (assuming test files are renamed with _dp suffix)
test_openai_text_dp_SOURCES = test_openai_text_dp.c
test_openai_multimodal_dp_SOURCES = test_openai_multimodal_dp.c
test_gemini_text_dp_SOURCES = test_gemini_text_dp.c
test_gemini_multimodal_dp_SOURCES = test_gemini_multimodal_dp.c
test_openai_streaming_dp_SOURCES = test_openai_streaming_dp.c
test_gemini_streaming_dp_SOURCES = test_gemini_streaming_dp.c

# Link against our library and its dependencies
# LDADD is used for all `check_PROGRAMS`
LDADD = ../src/libdisasterparty.la # Corrected library name

# AM_CPPFLAGS for tests to find the library header and dependency headers
AM_CPPFLAGS = $(CURL_CFLAGS) $(CJSON_CFLAGS) -I$(top_srcdir)/src

# Specify tests to run with 'make check'
TESTS = $(check_PROGRAMS)

# Prevent tests from being installed by default
noinst_PROGRAMS = $(check_PROGRAMS)

