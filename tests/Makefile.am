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
    test_anthropic_multimodal_dp \
    test_anthropic_list_models_dp \
    test_openai_streaming_multimodal_dp \
    test_gemini_streaming_multimodal_dp \
    test_anthropic_streaming_multimodal_dp \
    test_anthropic_streaming_multimodal_detailed_dp \
    test_serialization_dp \
    test_inline_multimodal_dp \
    test_error_handling_dp \
    test_parameters_dp \
    test_anthropic_token_counting_dp \
    test_file_attachments_dp

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
test_anthropic_multimodal_dp_SOURCES = test_anthropic_multimodal_dp.c
test_anthropic_list_models_dp_SOURCES = test_anthropic_list_models_dp.c
test_openai_streaming_multimodal_dp_SOURCES = test_openai_streaming_multimodal_dp.c
test_gemini_streaming_multimodal_dp_SOURCES = test_gemini_streaming_multimodal_dp.c
test_anthropic_streaming_multimodal_dp_SOURCES = test_anthropic_streaming_multimodal_dp.c
test_inline_multimodal_dp_SOURCES = test_inline_multimodal_dp.c
test_error_handling_dp_SOURCES = test_error_handling_dp.c
test_parameters_dp_SOURCES = test_parameters_dp.c
test_anthropic_streaming_multimodal_detailed_dp_SOURCES = test_anthropic_streaming_multimodal_detailed_dp.c
test_anthropic_token_counting_dp_SOURCES = test_anthropic_token_counting_dp.c
test_file_attachments_dp_SOURCES = test_file_attachments_dp.c


LDADD = ../src/libdisasterparty.la $(CURL_LIBS) $(CJSON_LIBS)

AM_CPPFLAGS = $(CURL_CFLAGS) $(CJSON_CFLAGS) -I$(top_srcdir)/src

TESTS = $(check_PROGRAMS)
noinst_PROGRAMS = $(check_PROGRAMS)
