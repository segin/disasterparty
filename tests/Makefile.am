# Programs to build for tests
check_PROGRAMS = \
    test_openai_text_dp \
    test_openai_multimodal_dp \
    test_gemini_text_dp \
    test_gemini_multimodal_dp \
    test_openai_streaming_dp \
    test_gemini_streaming_dp \
    test_openai_list_models_dp \
    test_gemini_list_models_dp \
    test_anthropic_text_dp \
    test_anthropic_streaming_dp \
    test_anthropic_streaming_detailed_dp \
    test_serialization_dp

# Sources for each test program
test_openai_text_dp_SOURCES = test_openai_text_dp.c
test_openai_multimodal_dp_SOURCES = test_openai_multimodal_dp.c
test_gemini_text_dp_SOURCES = test_gemini_text_dp.c
test_gemini_multimodal_dp_SOURCES = test_gemini_multimodal_dp.c
test_openai_streaming_dp_SOURCES = test_openai_streaming_dp.c
test_gemini_streaming_dp_SOURCES = test_gemini_streaming_dp.c
test_openai_list_models_dp_SOURCES = test_openai_list_models_dp.c
test_gemini_list_models_dp_SOURCES = test_gemini_list_models_dp.c
test_anthropic_text_dp_SOURCES = test_anthropic_text_dp.c
test_anthropic_streaming_dp_SOURCES = test_anthropic_streaming_dp.c
test_anthropic_streaming_detailed_dp_SOURCES = test_anthropic_streaming_detailed_dp.c
test_serialization_dp_SOURCES = test_serialization_dp.c


LDADD = ../src/libdisasterparty.la

AM_CPPFLAGS = $(CURL_CFLAGS) $(CJSON_CFLAGS) -I$(top_srcdir)/src

TESTS = $(check_PROGRAMS)
noinst_PROGRAMS = $(check_PROGRAMS)
