#ifndef DP_PRIVATE_H
#define DP_PRIVATE_H

#include <stdint.h>
#include "disasterparty.h"
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <stdbool.h>

// Default base URLs - declared as extern, defined in dp_constants.c
extern const char* DEFAULT_OPENAI_API_BASE_URL;
extern const char* DEFAULT_GEMINI_API_BASE_URL;
extern const char* DEFAULT_ANTHROPIC_API_BASE_URL;

struct dp_context_s {
    dp_provider_type_t provider;
    char* api_key;
    char* api_base_url;
    char* user_agent;
    dp_token_param_type_t token_param_preference;
    uint64_t features;
};

typedef struct {
    char* memory;
    size_t size;
} memory_struct_t;

typedef dp_anthropic_stream_callback_t dp_detailed_stream_callback_t;

typedef struct {
    dp_stream_callback_t user_callback;
    dp_detailed_stream_callback_t detailed_callback;
    void* user_data;
    char* buffer;
    size_t buffer_size;
    size_t buffer_capacity;
    dp_provider_type_t provider;
    char* finish_reason_capture;
    bool stop_streaming_signal;
    char* accumulated_error_during_stream;
    uint64_t features;
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
    bool is_thinking;
} anthropic_stream_processor_t;

// --- Shared Internal Function Prototypes ---

// Payload Builders (disasterparty.c)
char* dpinternal_build_openai_json_payload_with_cjson(const dp_request_config_t* request_config, const dp_context_t* context);
char* dpinternal_build_gemini_json_payload_with_cjson(const dp_request_config_t* request_config);
char* dpinternal_build_anthropic_json_payload_with_cjson(const dp_request_config_t* request_config);
char* dpinternal_build_gemini_count_tokens_json_payload_with_cjson(const dp_request_config_t* request_config);
char* dpinternal_build_anthropic_count_tokens_json_payload_with_cjson(const dp_request_config_t* request_config);

// Response processing (disasterparty.c)
bool dpinternal_parse_response_content(const dp_context_t* context, const char* json_response_str, dp_response_part_t** parts_out, size_t* num_parts_out, char** finish_reason_out);
bool dpinternal_is_token_parameter_error(const char* error_response, long http_status);

// OpenAI Fallback logic (disasterparty.c)
CURLcode dpinternal_perform_openai_request_with_fallback(CURL* curl, dp_context_t* context, 
                                                                const dp_request_config_t* request_config,
                                                                memory_struct_t* chunk_mem,
                                                                long* http_status_code);
CURLcode dpinternal_perform_openai_streaming_request_with_fallback(CURL* curl, dp_context_t* context, 
                                                                        const dp_request_config_t* request_config,
                                                                        stream_processor_t* processor,
                                                                        long* http_status_code);
CURLcode dpinternal_perform_openai_detailed_streaming_request_with_fallback(CURL* curl, dp_context_t* context, 
                                                                        const dp_request_config_t* request_config,
                                                                        anthropic_stream_processor_t* processor,
                                                                        long* http_status_code);

// Image Generation Payload Builders
char* dpinternal_build_openai_image_generation_payload_with_cjson(const dp_image_generation_config_t* config);
char* dpinternal_build_google_image_generation_payload_with_cjson(const dp_image_generation_config_t* config, const dp_context_t* context);

// cURL Callbacks (dp_utils.c)
size_t dpinternal_write_memory_callback(void* contents, size_t size, size_t nmemb, void* userp);
size_t dpinternal_streaming_write_callback(void* contents, size_t size, size_t nmemb, void* userp);
size_t dpinternal_anthropic_detailed_stream_write_callback(void* contents, size_t size, size_t nmemb, void* userp);
size_t dpinternal_openai_detailed_stream_write_callback(void* contents, size_t size, size_t nmemb, void* userp);

// Utilities (dp_utils.c)
char* dpinternal_strdup(const char* s);
int dpinternal_safe_asprintf(char** strp, const char* fmt, ...);

// File handling helpers
char* dpinternal_get_mime_type(const char* filename);
char* dpinternal_encode_base64(const unsigned char* data, size_t input_length);
unsigned char* dpinternal_decode_base64(const char* base64_data, size_t* output_length);
bool dpinternal_write_base64_to_file(const char* path, const char* base64_data, const char* filename);
char* dpinternal_encode_file_to_base64(const char* file_path);
bool dpinternal_message_add_file_from_path(dp_message_t* message, const char* file_path, const char* mime_type_override);

#endif // DP_PRIVATE_H
