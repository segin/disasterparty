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
#define DP_VERSION "0.6.0"
#define DP_VERSION_MAJOR 0
#define DP_VERSION_MINOR 6 
#define DP_VERSION_PATCH 0 


/**
 * @brief Enumeration for supported LLM API providers.
 */
typedef enum {
    DP_PROVIDER_OPENAI_COMPATIBLE, 
    DP_PROVIDER_GOOGLE_GEMINI,
    DP_PROVIDER_ANTHROPIC  // Claude API (formerly Anthropic API)
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
 * @brief Tool type enumeration.
 */
typedef enum {
    DP_TOOL_TYPE_FUNCTION
} dp_tool_type_t;

/**
 * @brief Tool choice type enumeration.
 */
typedef enum {
    DP_TOOL_CHOICE_AUTO,
    DP_TOOL_CHOICE_ANY,
    DP_TOOL_CHOICE_TOOL,
    DP_TOOL_CHOICE_NONE
} dp_tool_choice_type_t;

/**
 * @brief Represents a part of a multimodal message content.
 */
typedef enum {
    DP_CONTENT_PART_TEXT, 
    DP_CONTENT_PART_IMAGE_URL,
    DP_CONTENT_PART_IMAGE_BASE64,
    DP_CONTENT_PART_FILE_DATA,
    DP_CONTENT_PART_FILE_REFERENCE,
    DP_CONTENT_PART_TOOL_CALL,
    DP_CONTENT_PART_TOOL_RESULT,
    DP_CONTENT_PART_THINKING
} dp_content_part_type_t;

/**
 * @brief Enumeration for token parameter preference in OpenAI-compatible APIs.
 */
typedef enum {
    DP_TOKEN_PARAM_MAX_COMPLETION_TOKENS,  // Modern parameter (preferred)
    DP_TOKEN_PARAM_MAX_TOKENS              // Legacy parameter (fallback)
} dp_token_param_type_t; 

typedef struct {
    char* name;
    char* description;
    char* parameters_json_schema; 
} dp_tool_function_t;

typedef struct {
    dp_tool_type_t type;
    dp_tool_function_t function;
} dp_tool_definition_t;

typedef struct {
    dp_tool_choice_type_t type;
    char* tool_name; 
} dp_tool_choice_t;

typedef struct {
    dp_content_part_type_t type;
    char* text;
    char* image_url;
    struct {
        char* mime_type;
        char* data; 
    } image_base64;
    struct {
        char* mime_type;
        char* data;
        char* filename;
    } file_data;
    struct {
        char* file_id;
        char* mime_type;
    } file_reference;
    struct {
        char* id;
        char* function_name;
        char* arguments_json;
    } tool_call;
    struct {
        char* tool_call_id;
        char* content;
        bool is_error;
    } tool_result;
    struct {
        char* thinking;
        char* signature;
    } thinking;
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
    double top_p;                   // New parameter for nucleus sampling
    int top_k;                      // New parameter for top-k sampling
    const char** stop_sequences;    // New: Array of stop sequences
    size_t num_stop_sequences;      // New: Number of stop sequences
    const dp_tool_definition_t* tools; // New: Array of tool definitions
    size_t num_tools;                  // New: Number of tools
    dp_tool_choice_t tool_choice;      // New: Tool choice configuration
    struct {
        bool enabled;
        int budget_tokens;
    } thinking;
    const char* reasoning_effort;
} dp_request_config_t; 

typedef struct {
    dp_content_part_type_t type; 
    char* text;
    struct {
        char* id;
        char* function_name;
        char* arguments_json;
    } tool_call;
    struct {
        char* thinking;
        char* signature;
    } thinking;
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

typedef struct {
    char* file_id;
    char* display_name;
    char* mime_type;
    long size_bytes;
    char* create_time;
    char* uri;
    long http_status_code;
    char* error_message;
} dp_file_t;

typedef struct {
    char* url;
    char* base64_json;
    char* revised_prompt;
} dp_image_data_t;

typedef struct {
    dp_image_data_t* images;
    size_t num_images;
    long created;
    char* error_message;
    long http_status_code;
} dp_image_generation_response_t;

typedef struct {
    const char* prompt;
    const char* model;
    const char* size;
    const char* quality;
    const char* style;
    int n;
    const char* response_format;
} dp_image_generation_config_t;

typedef enum {
    DP_ANTHROPIC_EVENT_UNKNOWN,
    DP_ANTHROPIC_EVENT_MESSAGE_START,
    DP_ANTHROPIC_EVENT_CONTENT_BLOCK_START,
    DP_ANTHROPIC_EVENT_PING,
    DP_ANTHROPIC_EVENT_CONTENT_BLOCK_DELTA,
    DP_ANTHROPIC_EVENT_CONTENT_BLOCK_STOP,
    DP_ANTHROPIC_EVENT_MESSAGE_DELTA,
    DP_ANTHROPIC_EVENT_MESSAGE_STOP,
    DP_ANTHROPIC_EVENT_ERROR,
    DP_ANTHROPIC_EVENT_THINKING_DELTA
} dp_anthropic_event_type_t;  // Claude API event types (maintains "anthropic" naming for backwards compatibility)

typedef struct {
    dp_anthropic_event_type_t event_type; 
    const char* raw_json_data;            
} dp_anthropic_stream_event_t;  // Claude API stream event (maintains "anthropic" naming for backwards compatibility)

typedef int (*dp_stream_callback_t)(const char* token, 
                                    void* user_data,
                                    bool is_final_chunk,
                                    const char* error_during_stream);

// Generic aliases for detailed streaming
#define DP_EVENT_UNKNOWN              DP_ANTHROPIC_EVENT_UNKNOWN
#define DP_EVENT_MESSAGE_START        DP_ANTHROPIC_EVENT_MESSAGE_START
#define DP_EVENT_CONTENT_BLOCK_START  DP_ANTHROPIC_EVENT_CONTENT_BLOCK_START
#define DP_EVENT_PING                 DP_ANTHROPIC_EVENT_PING
#define DP_EVENT_CONTENT_BLOCK_DELTA  DP_ANTHROPIC_EVENT_CONTENT_BLOCK_DELTA
#define DP_EVENT_CONTENT_BLOCK_STOP   DP_ANTHROPIC_EVENT_CONTENT_BLOCK_STOP
#define DP_EVENT_MESSAGE_DELTA        DP_ANTHROPIC_EVENT_MESSAGE_DELTA
#define DP_EVENT_MESSAGE_STOP         DP_ANTHROPIC_EVENT_MESSAGE_STOP
#define DP_EVENT_ERROR                DP_ANTHROPIC_EVENT_ERROR
#define DP_EVENT_THINKING_DELTA       DP_ANTHROPIC_EVENT_THINKING_DELTA

typedef dp_anthropic_event_type_t dp_stream_event_type_t;
typedef dp_anthropic_stream_event_t dp_stream_event_t;

typedef int (*dp_anthropic_stream_callback_t)(const dp_anthropic_stream_event_t* event,
                                              void* user_data,
                                              const char* error_during_stream);  // Claude API callback (maintains "anthropic" naming for backwards compatibility)

typedef struct dp_context_s dp_context_t; 

/**
 * @brief Advanced feature flags that can be enabled.
 */
typedef enum {
    DP_FEATURE_THINKING = 1,
    // Future features can be added here
} dp_feature_t;

/**
 * @brief Enables advanced features for the context.
 * 
 * This function takes a variable number of features to enable,
 * terminated by 0.
 * 
 * Example: dp_enable_advanced_features(ctx, DP_FEATURE_THINKING, 0);
 */
void dp_enable_advanced_features(dp_context_t* context, ...);

dp_context_t* dp_init_context(dp_provider_type_t provider, 
                              const char* api_key,
                              const char* api_base_url);

dp_context_t* dp_init_context_with_app_info(dp_provider_type_t provider, 
                                             const char* api_key,
                                             const char* api_base_url,
                                             const char* app_name,
                                             const char* app_version);

void dp_destroy_context(dp_context_t* context);

int dp_perform_completion(dp_context_t* context,
                          const dp_request_config_t* request_config,
                          dp_response_t* response);

int dp_perform_streaming_completion(dp_context_t* context,
                                    const dp_request_config_t* request_config,
                                    dp_stream_callback_t callback, 
                                    void* user_data,
                                    dp_response_t* response);

/**
 * @brief Detailed streaming completion for all providers.
 * 
 * This is an alias for dp_perform_anthropic_streaming_completion, which is now
 * generalized to support detailed events across different providers.
 */
int dp_perform_detailed_streaming_completion(dp_context_t* context,
                                              const dp_request_config_t* request_config,
                                              dp_anthropic_stream_callback_t callback,
                                              void* user_data,
                                              dp_response_t* response);

int dp_perform_anthropic_streaming_completion(dp_context_t* context,
                                              const dp_request_config_t* request_config,
                                              dp_anthropic_stream_callback_t anthropic_callback,
                                              void* user_data,
                                              dp_response_t* response);  // Claude API streaming (maintains "anthropic" naming for backwards compatibility)

int dp_list_models(dp_context_t* context, dp_model_list_t** model_list_out);

int dp_count_tokens(dp_context_t* context,
                    const dp_request_config_t* request_config,
                    size_t* token_count_out);

void dp_free_model_list(dp_model_list_t* model_list);

void dp_free_response_content(dp_response_t* response);
void dp_free_messages(dp_message_t* messages, size_t num_messages);

bool dp_message_add_text_part(dp_message_t* message, const char* text);
bool dp_message_add_image_url_part(dp_message_t* message, const char* image_url);
bool dp_message_add_base64_image_part(dp_message_t* message, const char* mime_type, const char* base64_data);
bool dp_message_add_file_data_part(dp_message_t* message, const char* mime_type, const char* base64_data, const char* filename);
bool dp_message_add_file_reference_part(dp_message_t* message, const char* file_id, const char* mime_type);
bool dp_message_add_tool_call_part(dp_message_t* message, const char* id, const char* function_name, const char* arguments_json);
bool dp_message_add_tool_result_part(dp_message_t* message, const char* tool_call_id, const char* content, bool is_error);
bool dp_message_add_thinking_part(dp_message_t* message, const char* thinking, const char* signature);

const char* dp_get_version(void);

int dp_serialize_messages_to_json_str(const dp_message_t* messages, size_t num_messages, char** json_str_out);
int dp_deserialize_messages_from_json_str(const char* json_str, dp_message_t** messages_out, size_t* num_messages_out);
int dp_serialize_messages_to_file(const dp_message_t* messages, size_t num_messages, const char* path);
int dp_deserialize_messages_from_file(const char* path, dp_message_t** messages_out, size_t* num_messages_out);

int dp_upload_file(dp_context_t* context, const char* file_path, const char* mime_type, dp_file_t** file_out);
void dp_free_file(dp_file_t* file);

int dp_generate_image(dp_context_t* context, const dp_image_generation_config_t* config, dp_image_generation_response_t* response);
void dp_free_image_generation_response(dp_image_generation_response_t* response);

#endif // DISASTERPARTY_H

