#ifndef DISASTERPARTY_H
#define DISASTERPARTY_H

#include <stddef.h>
#include <stdbool.h>

// Forward declaration for libcurl and cJSON
typedef void CURL;
typedef struct cJSON cJSON;

/**
 * @brief Library version string for Disaster Party.
 */
#define DP_VERSION "0.2.0" // Updated version
#define DP_VERSION_MAJOR 0
#define DP_VERSION_MINOR 2 // Updated version
#define DP_VERSION_PATCH 0 // Updated version


/**
 * @brief Enumeration for supported LLM API providers.
 */
typedef enum {
    DP_PROVIDER_OPENAI_COMPATIBLE,
    DP_PROVIDER_GOOGLE_GEMINI,
    DP_PROVIDER_ANTHROPIC // New provider
} dp_provider_type_t;

/**
 * @brief Enumeration for the role in a message.
 */
typedef enum {
    DP_ROLE_SYSTEM,
    DP_ROLE_USER,
    DP_ROLE_ASSISTANT,
    DP_ROLE_TOOL
} dp_message_role_t;

/**
 * @brief Represents a part of a multimodal message content.
 */
typedef enum {
    DP_CONTENT_PART_TEXT,
    DP_CONTENT_PART_IMAGE_URL,
    DP_CONTENT_PART_IMAGE_BASE64
} dp_content_part_type_t;

typedef struct {
    dp_content_part_type_t type;
    char* text;
    char* image_url;
    struct {
        char* mime_type;
        char* data;
    } image_base64;
} dp_content_part_t;

typedef struct {
    dp_message_role_t role;
    dp_content_part_t* parts;
    size_t num_parts;
} dp_message_t;

typedef struct {
    const char* model;
    dp_message_t* messages;
    size_t num_messages;
    const char* system_prompt; // Optional system prompt, especially for Anthropic
    double temperature;
    int max_tokens;
    bool stream;
} dp_request_config_t;

typedef struct {
    dp_content_part_type_t type;
    char* text;
} dp_response_part_t;

typedef struct {
    dp_response_part_t* parts;
    size_t num_parts;
    char* error_message;
    long http_status_code;
    char* finish_reason;
} dp_response_t;

typedef struct {
    char* model_id;
    char* display_name;
    char* version;
    char* description;
    long input_token_limit;
    long output_token_limit;
} dp_model_info_t;

typedef struct {
    dp_model_info_t* models;
    size_t count;
    char* error_message;
    long http_status_code;
} dp_model_list_t;

/**
 * @brief Enumeration for Anthropic stream event types.
 */
typedef enum {
    DP_ANTHROPIC_EVENT_UNKNOWN,
    DP_ANTHROPIC_EVENT_MESSAGE_START,
    DP_ANTHROPIC_EVENT_CONTENT_BLOCK_START,
    DP_ANTHROPIC_EVENT_PING,
    DP_ANTHROPIC_EVENT_CONTENT_BLOCK_DELTA,
    DP_ANTHROPIC_EVENT_CONTENT_BLOCK_STOP,
    DP_ANTHROPIC_EVENT_MESSAGE_DELTA,
    DP_ANTHROPIC_EVENT_MESSAGE_STOP,
    DP_ANTHROPIC_EVENT_ERROR
} dp_anthropic_event_type_t;

/**
 * @brief Structure to hold a detailed Anthropic stream event.
 */
typedef struct {
    dp_anthropic_event_type_t event_type; // The type of the SSE event
    const char* raw_json_data;            // The raw JSON string from the 'data:' line of the SSE event
    // Future: Could include a pre-parsed cJSON* object, but that complicates memory management for the callback.
    // Raw JSON is safer for the callback to parse as needed.
} dp_anthropic_stream_event_t;

typedef int (*dp_stream_callback_t)(const char* token,
                                    void* user_data,
                                    bool is_final_chunk,
                                    const char* error_during_stream);

/**
 * @brief Callback function type for handling detailed Anthropic streamed events.
 *
 * @param event A pointer to the dp_anthropic_stream_event_t structure containing
 * the event type and its raw JSON data. The `raw_json_data` string
 * is valid only for the duration of this callback; copy if needed.
 * @param user_data A pointer to user-defined data.
 * @param error_during_stream If an error occurred *during* the stream processing
 * (e.g., parsing SSE line), this will contain an error message.
 * @return Return 0 to continue streaming, non-zero to attempt to stop streaming.
 */
typedef int (*dp_anthropic_stream_callback_t)(const dp_anthropic_stream_event_t* event,
                                              void* user_data,
                                              const char* error_during_stream);

typedef struct dp_context_s dp_context_t;

dp_context_t* dp_init_context(dp_provider_type_t provider,
                              const char* api_key,
                              const char* api_base_url);

void dp_destroy_context(dp_context_t* context);

int dp_perform_completion(dp_context_t* context,
                          const dp_request_config_t* request_config,
                          dp_response_t* response);

int dp_perform_streaming_completion(dp_context_t* context,
                                    const dp_request_config_t* request_config,
                                    dp_stream_callback_t callback, // Generic text-token callback
                                    void* user_data,
                                    dp_response_t* response);

/**
 * @brief Performs a streaming completion request specifically for Anthropic,
 * providing detailed SSE events to the callback.
 *
 * @param context An initialized Disaster Party context (must be DP_PROVIDER_ANTHROPIC).
 * @param request_config Configuration for the request. `request_config->stream` must be true.
 * @param anthropic_callback The Anthropic-specific callback function.
 * @param user_data User-defined data to be passed to the callback.
 * @param response Pointer to a response structure for final status/initial errors.
 * @return 0 if streaming was successful, -1 on setup error or if provider is not Anthropic.
 */
int dp_perform_anthropic_streaming_completion(dp_context_t* context,
                                              const dp_request_config_t* request_config,
                                              dp_anthropic_stream_callback_t anthropic_callback,
                                              void* user_data,
                                              dp_response_t* response);

int dp_list_models(dp_context_t* context, dp_model_list_t** model_list_out);

void dp_free_model_list(dp_model_list_t* model_list);

void dp_free_response_content(dp_response_t* response);
void dp_free_messages(dp_message_t* messages, size_t num_messages);

bool dp_message_add_text_part(dp_message_t* message, const char* text);
bool dp_message_add_image_url_part(dp_message_t* message, const char* image_url);
bool dp_message_add_base64_image_part(dp_message_t* message, const char* mime_type, const char* base64_data);

const char* dp_get_version(void);

#endif // DISASTERPARTY_H
