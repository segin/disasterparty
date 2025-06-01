# Programs to build for tests
check_PROGRAMS = \
    test_openai_text_dpt \
    test_openai_multimodal_dpt \
    test_gemini_text_dpt \
    test_gemini_multimodal_dpt \
    test_openai_streaming_dpt \
    test_gemini_streaming_dpt

# Sources for each test program
test_openai_text_dpt_SOURCES = test_openai_text_dpt.c
test_openai_multimodal_dpt_SOURCES = test_openai_multimodal_dpt.c
test_gemini_text_dpt_SOURCES = test_gemini_text_dpt.c
test_gemini_multimodal_dpt_SOURCES = test_gemini_multimodal_dpt.c
test_openai_streaming_dpt_SOURCES = test_openai_streaming_dpt.c
test_gemini_streaming_dpt_SOURCES = test_gemini_streaming_dpt.c

# Link against our library and libcurl
# LDADD is used for all `check_PROGRAMS` if not specified per program
LDADD = ../src/libdiasterparty.la $(CURL_LIBS)

# AM_CPPFLAGS for tests to find the library header from top_srcdir
AM_CPPFLAGS = -I$(top_srcdir)/src

# Specify tests to run with 'make check'
TESTS = $(check_PROGRAMS)

# Prevent tests from being installed
noinst_PROGRAMS = $(check_PROGRAMS)

