#ifndef DP_PRIVATE_H
#define DP_PRIVATE_H

#include "disasterparty.h"
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <stdbool.h>

// Default base URLs - declared as extern, defined in dp_constants.c
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
    dp_token_param_type_t token_param_preference;
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

// Utility functions (dp_utils.c)
char* dpinternal_strdup(const char* s);
int dpinternal_safe_asprintf(char** strp, const char* fmt, ...);
size_t dpinternal_write_memory_callback(void* contents, size_t size, size_t nmemb, void* userp);

// JSON payload builders (dp_request.c)
char* dpinternal_build_openai_json_payload_with_cjson(const dp_request_config_t* request_config, const dp_context_t* context);
char* dpinternal_build_gemini_json_payload_with_cjson(const dp_request_config_t* request_config);
char* dpinternal_build_anthropic_json_payload_with_cjson(const dp_request_config_t* request_config);
char* dpinternal_build_gemini_count_tokens_json_payload_with_cjson(const dp_request_config_t* request_config);
char* dpinternal_build_anthropic_count_tokens_json_payload_with_cjson(const dp_request_config_t* request_config);

// Response processing (dp_request.c)
char* dpinternal_extract_text_from_full_response_with_cjson(const char* json_response_str, dp_provider_type_t provider, char** finish_reason_out);
bool dpinternal_is_token_parameter_error(const char* error_response, long http_status);
CURLcode dpinternal_perform_openai_request_with_fallback(CURL* curl, dp_context_t* context, 
                                                        const dp_request_config_t* request_config,
                                                        memory_struct_t* chunk_mem,
                                                        long* http_status_code);
CURLcode dpinternal_perform_openai_streaming_request_with_fallback(CURL* curl, dp_context_t* context, 
                                                                  const dp_request_config_t* request_config,
                                                                  stream_processor_t* processor,
                                                                  long* http_status_code);

// Streaming callbacks (dp_stream.c)
size_t dpinternal_streaming_write_callback(void* contents, size_t size, size_t nmemb, void* userp);
size_t dpinternal_anthropic_detailed_stream_write_callback(void* contents, size_t size, size_t nmemb, void* userp);

// Message handling internal functions (dp_message.c)
bool dpinternal_message_add_part_internal(dp_message_t* message, dp_content_part_type_t type,
                                         const char* text_content, const char* image_url_content,
                                         const char* mime_type_content, const char* base64_data_content,
                                         const char* filename_content);

#endif // DP_PRIVATE_H