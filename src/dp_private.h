#ifndef DP_PRIVATE_H
#define DP_PRIVATE_H

#include "disasterparty.h"
#include <curl/curl.h>
#include <cjson/cJSON.h>

// Default base URLs
extern const char* DEFAULT_OPENAI_API_BASE_URL;
extern const char* DEFAULT_GEMINI_API_BASE_URL;
extern const char* DEFAULT_ANTHROPIC_API_BASE_URL;
extern const char* DISASTERPARTY_USER_AGENT;

// --- Internal Structures ---

struct dp_context_s {
    dp_provider_type_t provider;
    char* api_key;
    char* api_base_url;
    char* user_agent;
};

typedef struct {
    char* memory;
    size_t size;
} memory_struct_t;

typedef struct {
    dp_stream_callback_t user_callback;
    void* user_data;
    char* buffer;
    size_t buffer_size;
    size_t buffer_capacity;
    dp_provider_type_t provider;
    char* finish_reason_capture;
    bool stop_streaming_signal;
    char* accumulated_error_during_stream;
} stream_processor_t;

typedef struct {
    dp_anthropic_stream_callback_t anthropic_user_callback;
    void* user_data;
    char* buffer;
    size_t buffer_size;
    size_t buffer_capacity;
    char* finish_reason_capture;
    bool stop_streaming_signal;
    char* accumulated_error_during_stream;
} anthropic_stream_processor_t;

// --- Shared Internal Function Prototypes ---

char* dp_internal_strdup(const char* s);
size_t write_memory_callback(void* contents, size_t size, size_t nmemb, void* userp);

char* build_openai_json_payload_with_cjson(const dp_request_config_t* request_config);
char* build_gemini_json_payload_with_cjson(const dp_request_config_t* request_config);
char* build_anthropic_json_payload_with_cjson(const dp_request_config_t* request_config);
char* build_gemini_count_tokens_json_payload_with_cjson(const dp_request_config_t* request_config);
char* build_anthropic_count_tokens_json_payload_with_cjson(const dp_request_config_t* request_config);

char* extract_text_from_full_response_with_cjson(const char* json_response_str, dp_provider_type_t provider, char** finish_reason_out);
size_t streaming_write_callback(void* contents, size_t size, size_t nmemb, void* userp);
size_t anthropic_detailed_stream_write_callback(void* contents, size_t size, size_t nmemb, void* userp);

#endif // DP_PRIVATE_H
