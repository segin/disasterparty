#ifndef DISASTERPARTY_H // Corrected include guard
#define DISASTERPARTY_H

#include <stddef.h> // For size_t
#include <stdbool.h> // For bool

// Forward declaration for libcurl and cJSON
typedef void CURL;
typedef struct cJSON cJSON;

/**
 * @brief Library version string for Disaster Party.
 */
#define DP_VERSION "0.1.0"
#define DP_VERSION_MAJOR 0
#define DP_VERSION_MINOR 1
#define DP_VERSION_PATCH 0


/**
 * @brief Enumeration for supported LLM API providers.
 */
typedef enum {
    DP_PROVIDER_OPENAI_COMPATIBLE, // Corrected prefix
    DP_PROVIDER_GOOGLE_GEMINI
} dp_provider_type_t; // Corrected type

/**
 * @brief Enumeration for the role in a message.
 */
typedef enum {
    DP_ROLE_SYSTEM, // Corrected prefix
    DP_ROLE_USER,
    DP_ROLE_ASSISTANT,
    DP_ROLE_TOOL
} dp_message_role_t; // Corrected type

/**
 * @brief Represents a part of a multimodal message content.
 */
typedef enum {
    DP_CONTENT_PART_TEXT, // Corrected prefix
    DP_CONTENT_PART_IMAGE_URL,
    DP_CONTENT_PART_IMAGE_BASE64
} dp_content_part_type_t; // Corrected type

typedef struct {
    dp_content_part_type_t type;
    char* text;
    char* image_url;
    struct {
        char* mime_type;
        char* data; // Base64 encoded string
    } image_base64;
} dp_content_part_t; // Corrected type

/**
 * @brief Represents a single message in a conversation.
 */
typedef struct {
    dp_message_role_t role;
    dp_content_part_t* parts;
    size_t num_parts;
} dp_message_t; // Corrected type

/**
 * @brief Configuration for an LLM API request.
 */
typedef struct {
    const char* model;
    dp_message_t* messages;
    size_t num_messages;
    double temperature;       // Sampling temperature.
    int max_tokens;           // Max tokens to generate.
    bool stream;              // True for streaming, false for single response.
} dp_request_config_t; // Corrected type

/**
 * @brief Represents a part of an LLM's response (for non-streaming).
 */
typedef struct {
    dp_content_part_type_t type; 
    char* text;
} dp_response_part_t; // Corrected type

/**
 * @brief Represents the overall response from an LLM.
 * For non-streaming, it contains the full response parts.
 * For streaming, it primarily contains status/error info after the stream.
 */
typedef struct {
    dp_response_part_t* parts; 
    size_t num_parts;          
    char* error_message;        
    long http_status_code;      
    char* finish_reason; // Populated for both streaming and non-streaming if available       
} dp_response_t; // Corrected type


/**
 * @brief Callback function type for handling streamed data from Disaster Party.
 *
 * @param token The piece of text (token or chunk) received from the stream.
 * This memory is managed by the library for the duration of the callback.
 * NULL if an error occurs during streaming or at the very end of the stream.
 * @param user_data A pointer to user-defined data, passed through from the streaming call.
 * @param is_final_chunk True if this is the last content-bearing chunk or an indication of stream end.
 * @param error_during_stream If an error occurred *during* the streaming process, this contains an error message.
 * If NULL and token is NULL and is_final_chunk is true, it's a clean stream end.
 * @return Return 0 to continue streaming, non-zero to attempt to stop streaming.
 */
typedef int (*dp_stream_callback_t)(const char* token, // Corrected type
                                    void* user_data,
                                    bool is_final_chunk,
                                    const char* error_during_stream);

/**
 * @brief Opaque context structure for managing Disaster Party LLM client state.
 */
typedef struct dp_context_s dp_context_t; // Corrected type

/**
 * @brief Initializes a Disaster Party LLM client context.
 *
 * @param provider The LLM provider type.
 * @param api_key The API key for authentication.
 * @param api_base_url Optional: Base URL for the API. If NULL, defaults are used.
 * @return A pointer to the initialized context, or NULL on failure.
 * The caller is responsible for freeing this context using dp_destroy_context.
 */
dp_context_t* dp_init_context(dp_provider_type_t provider, // Corrected functions
                              const char* api_key,
                              const char* api_base_url);

/**
 * @brief Destroys a Disaster Party LLM client context and frees associated resources.
 * @param context The context to destroy.
 */
void dp_destroy_context(dp_context_t* context);

/**
 * @brief Performs a non-streaming completion request to the LLM.
 * `request_config->stream` should be false.
 *
 * @param context The Disaster Party client context.
 * @param request_config Configuration for the request.
 * @param response Pointer to a response structure to be filled.
 * @return 0 on success, -1 on error.
 */
int dp_perform_completion(dp_context_t* context,
                          const dp_request_config_t* request_config,
                          dp_response_t* response);

/**
 * @brief Performs a streaming completion request to the LLM.
 * For OpenAI, `request_config->stream` must be true.
 * For Gemini, this function uses the :streamGenerateContent endpoint.
 *
 * @param context The Disaster Party client context.
 * @param request_config Configuration for the request.
 * @param callback The function to call for each received token/chunk.
 * @param user_data User-defined data to be passed to the callback.
 * @param response Pointer to a response structure for final status/errors.
 * @return 0 if streaming was successful (or errors handled by callback), -1 on setup error.
 */
int dp_perform_streaming_completion(dp_context_t* context,
                                    const dp_request_config_t* request_config,
                                    dp_stream_callback_t callback,
                                    void* user_data,
                                    dp_response_t* response);

/**
 * @brief Frees the content of a dp_response_t structure.
 * @param response The response structure whose content is to be freed.
 */
void dp_free_response_content(dp_response_t* response);

/**
 * @brief Frees the content of an array of dp_message_t, including their parts.
 * @param messages Array of messages.
 * @param num_messages Number of messages in the array.
 */
void dp_free_messages(dp_message_t* messages, size_t num_messages);

/**
 * @brief Adds a text part to a message.
 * @param message The message to modify.
 * @param text The text content.
 * @return true on success, false on failure.
 */
bool dp_message_add_text_part(dp_message_t* message, const char* text);

/**
 * @brief Adds an image URL part to a message.
 * @param message The message to modify.
 * @param image_url The URL of the image.
 * @return true on success, false on failure.
 */
bool dp_message_add_image_url_part(dp_message_t* message, const char* image_url);

/**
 * @brief Adds a base64 encoded image part to a message.
 * @param message The message to modify.
 * @param mime_type The MIME type of the image (e.g., "image/png").
 * @param base64_data The base64 encoded image data.
 * @return true on success, false on failure.
 */
bool dp_message_add_base64_image_part(dp_message_t* message, const char* mime_type, const char* base64_data);

/**
 * @brief Returns the version string of the Disaster Party library.
 * @return A constant string representing the library version (e.g., "0.1.0").
 */
const char* dp_get_version(void);

#endif // DISASTERPARTY_H

