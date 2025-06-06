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
#define DP_VERSION "0.2.0"
#define DP_VERSION_MAJOR 0
#define DP_VERSION_MINOR 2
#define DP_VERSION_PATCH 0


/**
 * @brief Enumeration for supported LLM API providers.
 */
typedef enum {
    DP_PROVIDER_OPENAI_COMPATIBLE,
    DP_PROVIDER_GOOGLE_GEMINI,
    DP_PROVIDER_ANTHROPIC
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
    const char* system_prompt;
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

typedef struct {
    dp_anthropic_event_type_t event_type;
    const char* raw_json_data;
} dp_anthropic_stream_event_t;

typedef int (*dp_stream_callback_t)(const char* token,
                                    void* user_data,
                                    bool is_final_chunk,
                                    const char* error_during_stream);

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
                                    dp_stream_callback_t callback,
                                    void* user_data,
                                    dp_response_t* response);

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

/**
 * @brief Serializes an array of messages to a JSON string.
 *
 * The caller is responsible for freeing the output string `*json_str_out` using `free()`.
 *
 * @param messages An array of `dp_message_t` structures.
 * @param num_messages The number of messages in the array.
 * @param json_str_out A pointer to a `char*` that will be updated to point to the new JSON string.
 * @return 0 on success, -1 on failure.
 */
int dp_serialize_messages_to_json_str(const dp_message_t* messages, size_t num_messages, char** json_str_out);

/**
 * @brief Deserializes a JSON string into an array of messages.
 *
 * The caller is responsible for freeing the output array `*messages_out` and its contents
 * using `dp_free_messages()`.
 *
 * @param json_str The null-terminated JSON string to parse.
 * @param messages_out A pointer to a `dp_message_t*` that will be updated to point to the new array of messages.
 * @param num_messages_out A pointer to a `size_t` that will be updated with the number of messages in the array.
 * @return 0 on success, -1 on failure (e.g., parse error).
 */
int dp_deserialize_messages_from_json_str(const char* json_str, dp_message_t** messages_out, size_t* num_messages_out);

/**
 * @brief Serializes an array of messages to a file.
 *
 * @param messages An array of `dp_message_t` structures.
 * @param num_messages The number of messages in the array.
 * @param path The file path to save the JSON to.
 * @return 0 on success, -1 on failure.
 */
int dp_serialize_messages_to_file(const dp_message_t* messages, size_t num_messages, const char* path);

/**
 * @brief Deserializes an array of messages from a file.
 *
 * @param path The file path to read the JSON from.
 * @param messages_out A pointer to a `dp_message_t*` that will be updated to point to the new array of messages.
 * @param num_messages_out A pointer to a `size_t` that will be updated with the number of messages in the array.
 * @return 0 on success, -1 on failure.
 */
int dp_deserialize_messages_from_file(const char* path, dp_message_t** messages_out, size_t* num_messages_out);


#endif // DISASTERPARTY_H
