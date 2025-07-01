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
    test_parameters_dp     test_gemini_file_attachment_dp     test_invalid_init_dp 
    test_null_or_empty_messages_dp     test_malformed_serialization_dp     test_streaming_callback_stop_dp 
    test_gemini_file_completion_dp 
    test_anthropic_all_event_types_dp     test_invalid_parameters_dp 
    test_large_payload_dp 
    test_free_functions_with_null_dp 
    test_multiprovider_multithread_dp 
    test_empty_api_key_dp

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
test_gemini_file_attachment_dp_SOURCES = test_gemini_file_attachment_dp.c
test_invalid_init_dp_SOURCES = test_invalid_init_dp.c
test_null_or_empty_messages_dp_SOURCES = test_null_or_empty_messages_dp.c
test_malformed_serialization_dp_SOURCES = test_malformed_serialization_dp.c
test_streaming_callback_stop_dp_SOURCES = test_streaming_callback_stop_dp.c
test_gemini_file_completion_dp_SOURCES = test_gemini_file_completion_dp.c
test_anthropic_all_event_types_dp_SOURCES = test_anthropic_all_event_types_dp.c
test_invalid_parameters_dp_SOURCES = test_invalid_parameters_dp.c
test_large_payload_dp_SOURCES = test_large_payload_dp.c
test_free_functions_with_null_dp_SOURCES = test_free_functions_with_null_dp.c
test_multiprovider_multithread_dp_SOURCES = test_multiprovider_multithread_dp.c
test_empty_api_key_dp_SOURCES = test_empty_api_key_dp.c


LDADD = ../src/libdisasterparty.la $(CURL_LIBS) $(CJSON_LIBS)

AM_CPPFLAGS = $(CURL_CFLAGS) $(CJSON_CFLAGS) -I$(top_srcdir)/src

# Explicitly add AM_CPPFLAGS to each test program's CFLAGS
test_openai_text_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_openai_multimodal_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_gemini_text_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_gemini_multimodal_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_openai_streaming_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_gemini_streaming_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_openai_list_models_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_gemini_list_models_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_anthropic_text_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_anthropic_streaming_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_anthropic_streaming_detailed_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_anthropic_multimodal_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_anthropic_list_models_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_openai_streaming_multimodal_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_gemini_streaming_multimodal_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_anthropic_streaming_multimodal_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_anthropic_streaming_multimodal_detailed_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_serialization_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_inline_multimodal_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_error_handling_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_parameters_dp_CPPFLAGS = $(AM_CPPFLAGS)
test_gemini_file_attachment_dp_CPPFLAGS = $(AM_CPPFLAGS)

TESTS = $(check_PROGRAMS)
noinst_PROGRAMS = $(check_PROGRAMS)
