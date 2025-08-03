# Disaster Party LLM Client Library - Full Documentation
**Version: 0.5.0**

## 1. Overview (disasterparty.7)

### NAME
**disasterparty** - overview of the Disaster Party LLM client library

### DESCRIPTION
The **Disaster Party** library provides a C interface for interacting with Large Language Model (LLM) APIs, currently supporting OpenAI-compatible endpoints, the Google Gemini API, and the Anthropic Claude API. It aims to simplify common tasks such as sending text or multimodal prompts and handling both regular (blocking) and streaming responses.

The library is designed to be relatively simple to integrate and use, leveraging **libcurl**(3) for HTTP(S) communication and **cJSON**(3) for robust JSON parsing and generation.

Key features include:
- Support for text-only and multimodal (text + image URL/base64 + file attachments) inputs.
- Handling of both non-streaming (full response at once) and streaming (token-by-token) completions.
- A dedicated, detailed streaming callback for Anthropic's event-based stream.
- Interface for listing available models from the supported providers.
- Helper functions for constructing request messages and serializing/deserializing conversations.
- Support for base64 encoded file attachments with various MIME types (PDFs, CSVs, text files, etc.).
- Token counting functionality for Gemini and Anthropic providers.
- Custom user-agent support for application identification.
- Management of API contexts and response data.

### SYNOPSIS
To use the Disaster Party library in your C program, include the main header file:
```c
#include <disasterparty.h>
```
When compiling and linking, you will typically need to link against `libdisasterparty`, `libcurl`, and `libcjson`. Using pkg-config is recommended if available:
```sh
gcc my_app.c $(pkg-config --cflags --libs disasterparty)
```

### GETTING STARTED
1.  Initialize a context using **dp_init_context**(3) or **dp_init_context_with_app_info**(3), specifying the provider, API key, and optionally your application information for custom user-agent strings.
2.  Create messages using helper functions like **dp_message_add_text_part**(3), **dp_message_add_image_url_part**(3), **dp_message_add_base64_image_part**(3), or **dp_message_add_file_data_part**(3).
3.  Configure your request using `dp_request_config_t`, setting the model, messages, temperature, etc.
4.  Perform the API call using either **dp_perform_completion**(3) for a blocking response or **dp_perform_streaming_completion**(3) for streaming. For Anthropic, you can use the detailed **dp_perform_anthropic_streaming_completion**(3).
5.  To list available models, use **dp_list_models**(3).
6.  To count tokens in your prompts (Gemini and Anthropic only), use **dp_count_tokens**(3).
7.  To save/load conversations, use **dp_serialize_messages_to_file**(3) and **dp_deserialize_messages_from_file**(3).
8.  Always free allocated resources using functions like **dp_free_response_content**(3), **dp_free_model_list**(3), **dp_free_messages**(3), and finally **dp_destroy_context**(3).

### ENVIRONMENT
The test programs and typical applications using this library expect API keys to be set in environment variables:
-   **OPENAI_API_KEY**: For OpenAI-compatible services.
-   **GEMINI_API_KEY**: For Google Gemini services.
-   **ANTHROPIC_API_KEY**: For Anthropic Claude services.

### USER-AGENT CUSTOMIZATION
The library supports custom user-agent strings to help identify your application in HTTP requests and API logs. This is particularly useful for:
-   **API Usage Tracking**: Providers can identify requests from your specific application
-   **Debugging**: Easier to trace requests in logs when troubleshooting
-   **Rate Limiting**: Some providers may apply different rate limits based on user-agent
-   **Analytics**: Track usage patterns for your application

**Default Behavior**: When using **dp_init_context**(3), the user-agent defaults to "disasterparty/VERSION".

**Custom User-Agent**: Use **dp_init_context_with_app_info**(3) to specify your application name and version. The resulting user-agent will be formatted as "AppName/AppVersion (disasterparty/VERSION)".

**Best Practices**:
-   Use descriptive application names without spaces
-   Follow semantic versioning for application versions
-   Keep user-agent strings concise and informative

### FILE ATTACHMENT CAPABILITIES
The library supports attaching various file types to messages using base64 encoding. This enables AI models to process documents, analyze data files, and work with multimedia content.

**Supported Workflows**:
-   **Direct Attachment**: Use **dp_message_add_file_data_part**(3) to attach base64-encoded file data directly to messages
-   **Multiple Files**: Attach multiple files to a single message by calling the function multiple times
-   **Mixed Content**: Combine file attachments with text and image content in the same message

**File Processing Requirements**:
-   Files must be base64 encoded before attachment
-   MIME type must be specified accurately
-   Optional filename can provide context to the AI model

**Provider-Specific Limitations**:
-   **Google Gemini**: Supports most file types, may use Files API for large files
-   **Anthropic**: Good support for text and document files, size limitations apply
-   **OpenAI-compatible**: Limited support, varies by model and endpoint

**File Size Considerations**:
-   Large files may exceed API payload limits
-   Base64 encoding increases file size by approximately 33%
-   Consider file compression for large documents when possible
-   Some providers have specific size limits per file or per request

**Security Notes**:
-   Only attach files you trust and have permission to share
-   Be aware that file content will be sent to third-party AI providers
-   Consider data privacy implications when attaching sensitive documents

### FILES
-   `<disasterparty.h>`: The main header file.
-   `libdisasterparty.so`: The shared library file.

### BUGS
Please report any bugs or issues by opening a ticket on the GitHub issue tracker:
[https://github.com/segin/disasterparty/issues](https://github.com/segin/disasterparty/issues)

### AUTHOR
Kirn Gill II <segin2005@gmail.com>
Gemini (Conceptualization and initial C code generation)

### SEE ALSO
**dp_init_context**(3), **dp_init_context_with_app_info**(3), **dp_perform_completion**(3), **dp_list_models**(3), **dp_count_tokens**(3), **dp_message_add_file_data_part**(3), **curl**(1), **cJSON**(3)

---
## 2. API Functions (section 3)

### dp_get_version
**NAME**
dp_get_version - get the version string of the Disaster Party library

**SYNOPSIS**
```c
#include <disasterparty.h>
const char *dp_get_version(void);
```

**DESCRIPTION**
Returns a constant string representing the current library version (e.g., "0.2.0"). This string should not be modified or freed.

---
### dp_init_context
**NAME**
dp_init_context - initialize a Disaster Party LLM client context

**SYNOPSIS**
```c
#include <disasterparty.h>
dp_context_t *dp_init_context(dp_provider_type_t provider, const char *api_key, const char *api_base_url);
```

**DESCRIPTION**
Allocates and initializes a new context for interacting with an LLM provider. This context stores the API provider type, API key, and base URL. The returned context must be freed using **dp_destroy_context**(3).

**PARAMETERS**
-   `provider`: The LLM provider (`DP_PROVIDER_OPENAI_COMPATIBLE`, `DP_PROVIDER_GOOGLE_GEMINI`, or `DP_PROVIDER_ANTHROPIC`).
-   `api_key`: The mandatory authentication key.
-   `api_base_url`: Optional. Overrides the default base URL if provided.

**RETURN VALUE**
Returns a pointer to the new `dp_context_t` on success, or `NULL` on error.

---
### dp_init_context_with_app_info
**NAME**
dp_init_context_with_app_info - initialize a Disaster Party LLM client context with custom application information

**SYNOPSIS**
```c
#include <disasterparty.h>
dp_context_t *dp_init_context_with_app_info(dp_provider_type_t provider, const char *api_key, const char *api_base_url, const char *app_name, const char *app_version);
```

**DESCRIPTION**
Allocates and initializes a new context for interacting with an LLM provider, with support for custom user-agent strings. This function allows applications to identify themselves in HTTP requests by providing application name and version information. The user-agent string will be constructed in the format "AppName/AppVersion (disasterparty/DP_VERSION)" when both app_name and app_version are provided, or "disasterparty/DP_VERSION" when app_name is NULL. The returned context must be freed using **dp_destroy_context**(3).

**PARAMETERS**
-   `provider`: The LLM provider (`DP_PROVIDER_OPENAI_COMPATIBLE`, `DP_PROVIDER_GOOGLE_GEMINI`, or `DP_PROVIDER_ANTHROPIC`).
-   `api_key`: The mandatory authentication key.
-   `api_base_url`: Optional. Overrides the default base URL if provided.
-   `app_name`: Optional. The name of your application (can be NULL).
-   `app_version`: Optional. The version of your application (can be NULL).

**RETURN VALUE**
Returns a pointer to the new `dp_context_t` on success, or `NULL` on error.

**EXAMPLE**
```c
// Initialize with custom app info
dp_context_t *ctx = dp_init_context_with_app_info(
    DP_PROVIDER_ANTHROPIC, 
    "your-api-key", 
    NULL, 
    "MyApp", 
    "1.0.0"
);
// User-agent will be: "MyApp/1.0.0 (disasterparty/0.5.0)"

// Initialize without app info (equivalent to dp_init_context)
dp_context_t *ctx2 = dp_init_context_with_app_info(
    DP_PROVIDER_ANTHROPIC, 
    "your-api-key", 
    NULL, 
    NULL, 
    NULL
);
// User-agent will be: "disasterparty/0.5.0"
```

---
### dp_destroy_context
**NAME**
dp_destroy_context - destroy a Disaster Party LLM client context

**SYNOPSIS**
```c
#include <disasterparty.h>
void dp_destroy_context(dp_context_t *context);
```

**DESCRIPTION**
Deallocates a `dp_context_t` previously created by **dp_init_context**(3) and all its associated resources. Passing `NULL` is a safe no-op.

---
### dp_perform_completion
**NAME**
dp_perform_completion - perform a non-streaming LLM completion request

**SYNOPSIS**
```c
#include <disasterparty.h>
int dp_perform_completion(dp_context_t *context, const dp_request_config_t *request_config, dp_response_t *response);
```

**DESCRIPTION**
Sends a blocking request to the configured LLM provider and waits for the full response. `request_config->stream` must be `false`. The `response` structure will be populated with the result and must be freed with **dp_free_response_content**(3).

**RETURN VALUE**
Returns `0` on success and `-1` on critical errors. The `response` struct contains detailed status and error messages.

---
### dp_perform_streaming_completion
**NAME**
dp_perform_streaming_completion - perform a streaming LLM completion request with a generic callback

**SYNOPSIS**
```c
#include <disasterparty.h>
int dp_perform_streaming_completion(dp_context_t *context, const dp_request_config_t *request_config, dp_stream_callback_t callback, void *user_data, dp_response_t *response);
```

**DESCRIPTION**
Sends a streaming request to the configured LLM provider. This function is intended for simple use cases where only the text content of the stream is needed. It invokes the provided `callback` with text tokens as they are received. For Anthropic, this function internally parses the detailed event stream and extracts only the text for the callback. For OpenAI and Anthropic, `request_config->stream` must be `true`.

**CALLBACK SIGNATURE**
`int (*dp_stream_callback_t)(const char* token, void* user_data, bool is_final_chunk, const char* error_during_stream);`
- Return `0` to continue, non-zero to stop.

**RETURN VALUE**
Returns `0` if the stream was successfully initiated and `-1` on setup error. The `response` struct contains the final status.

---
### dp_perform_anthropic_streaming_completion
**NAME**
dp_perform_anthropic_streaming_completion - perform a streaming completion request specifically for Anthropic, providing detailed SSE events to the callback.

**SYNOPSIS**
```c
#include <disasterparty.h>
int dp_perform_anthropic_streaming_completion(dp_context_t *context, const dp_request_config_t *request_config, dp_anthropic_stream_callback_t anthropic_callback, void *user_data, dp_response_t *response);
```

**DESCRIPTION**
Performs a streaming request to the Anthropic API and provides detailed, event-by-event information to the `anthropic_callback`. This gives the caller full control over handling different event types (`message_start`, `content_block_delta`, `message_stop`, etc.). The context provider must be `DP_PROVIDER_ANTHROPIC` and `request_config->stream` must be `true`.

**CALLBACK SIGNATURE**
`int (*dp_anthropic_stream_callback_t)(const dp_anthropic_stream_event_t* event, void* user_data, const char* error_during_stream);`
- Return `0` to continue, non-zero to stop.

**RETURN VALUE**
Returns `0` if the stream was successfully initiated and `-1` on setup error. The `response` struct contains the final status.

---
### dp_list_models
**NAME**
dp_list_models - list available models from the LLM provider

**SYNOPSIS**
```c
#include <disasterparty.h>
int dp_list_models(dp_context_t *context, dp_model_list_t **model_list_out);
```

**DESCRIPTION**
Retrieves a list of available models from the configured provider. The caller is responsible for freeing the allocated `*model_list_out` using **dp_free_model_list**(3). For Gemini, this function strips the `"models/"` prefix from model IDs.

**RETURN VALUE**
Returns `0` on success and `-1` on failure. The `*model_list_out` struct will contain the list or error details.

---
### dp_count_tokens
**NAME**
dp_count_tokens - count tokens in a prompt for supported LLM providers

**SYNOPSIS**
```c
#include <disasterparty.h>
int dp_count_tokens(dp_context_t *context, const dp_request_config_t *request_config, size_t *token_count_out);
```

**DESCRIPTION**
Counts the number of tokens in a given prompt by calling the provider's token counting API endpoint. This function is useful for managing costs and ensuring prompts fit within model context windows. Token counting is currently supported for Google Gemini and Anthropic providers. For OpenAI-compatible providers, this function returns an error indicating the operation is not supported.

**PARAMETERS**
-   `context`: The initialized client context.
-   `request_config`: The request configuration containing the messages to count tokens for.
-   `token_count_out`: Pointer to a `size_t` variable that will receive the token count on success.

**RETURN VALUE**
Returns `0` on success with the token count stored in `*token_count_out`, or `-1` on failure. For OpenAI-compatible providers, this function always returns `-1` with an appropriate error message.

**PROVIDER SUPPORT**
-   **Google Gemini**: Uses the `/v1beta/models/{model}:countTokens` endpoint
-   **Anthropic**: Uses the `/v1/messages/count_tokens` endpoint
-   **OpenAI-compatible**: Not supported, returns error

**EXAMPLE**
```c
dp_request_config_t config = {0};
config.model = "claude-3-sonnet-20240229";
config.messages = messages;
config.num_messages = 1;

size_t token_count;
int result = dp_count_tokens(context, &config, &token_count);
if (result == 0) {
    printf("Token count: %zu\n", token_count);
} else {
    printf("Token counting failed or not supported\n");
}
```

---
### dp_free_model_list
**NAME**
dp_free_model_list - free a Disaster Party model list structure

**SYNOPSIS**
```c
#include <disasterparty.h>
void dp_free_model_list(dp_model_list_t *model_list);
```

**DESCRIPTION**
Deallocates all memory associated with a `dp_model_list_t` structure that was previously allocated by **dp_list_models**(3). Passing `NULL` is a safe no-op.

---
### dp_free_response_content
**NAME**
dp_free_response_content - free content of a Disaster Party response structure

**SYNOPSIS**
```c
#include <disasterparty.h>
void dp_free_response_content(dp_response_t *response);
```

**DESCRIPTION**
Deallocates the dynamically allocated memory within a `dp_response_t` structure. This should be called on any response structure passed to a completion function.

---
### dp_free_messages
**NAME**
dp_free_messages - free content of an array of Disaster Party messages

**SYNOPSIS**
```c
#include <disasterparty.h>
void dp_free_messages(dp_message_t *messages, size_t num_messages);
```

**DESCRIPTION**
Deallocates the dynamically allocated content within an array of `dp_message_t` structures (e.g., text, URLs, and the parts array itself). It does not free the `messages` array itself.

---
### Message Construction Helpers
**SYNOPSIS**
```c
#include <disasterparty.h>
bool dp_message_add_text_part(dp_message_t *message, const char *text);
bool dp_message_add_image_url_part(dp_message_t *message, const char *image_url);
bool dp_message_add_base64_image_part(dp_message_t *message, const char *mime_type, const char *base64_data);
bool dp_message_add_file_data_part(dp_message_t *message, const char *mime_type, const char *base64_data, const char *filename);
```

**DESCRIPTION**
These helper functions add a new content part of the specified type to a `dp_message_t` structure, reallocating memory as needed. They return `true` on success and `false` on failure.

---
### dp_message_add_base64_image_part
**NAME**
dp_message_add_base64_image_part - add a base64 encoded image part to a message

**SYNOPSIS**
```c
#include <disasterparty.h>
bool dp_message_add_base64_image_part(dp_message_t *message, const char *mime_type, const char *base64_data);
```

**DESCRIPTION**
Adds a new content part containing base64 encoded image data to a `dp_message_t` structure. The `mime_type` specifies the image format (e.g., "image/png", "image/jpeg").

**RETURN VALUE**
Returns `true` on success, `false` on failure (e.g., memory allocation error).

---
### dp_message_add_file_data_part
**NAME**
dp_message_add_file_data_part - add a base64 encoded file data part to a message

**SYNOPSIS**
```c
#include <disasterparty.h>
bool dp_message_add_file_data_part(dp_message_t *message, const char *mime_type, const char *base64_data, const char *filename);
```

**DESCRIPTION**
Adds a new content part containing base64 encoded file data to a `dp_message_t` structure. This function enables attaching various file types including PDFs, CSVs, text files, and other document formats directly to messages. The file data must be base64 encoded before calling this function. The `filename` parameter is optional and provides context to the AI model about the file.

**PARAMETERS**
-   `message`: The message to modify.
-   `mime_type`: The MIME type of the file (e.g., "text/plain", "application/pdf", "text/csv", "application/json").
-   `base64_data`: The base64 encoded file data string.
-   `filename`: Optional filename for the file (can be `NULL`).

**SUPPORTED FILE TYPES**
-   **Text files**: "text/plain", "text/csv", "text/markdown"
-   **Documents**: "application/pdf", "application/msword", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"
-   **Data files**: "application/json", "application/xml", "text/xml"
-   **Images**: "image/png", "image/jpeg", "image/gif", "image/webp"
-   **Archives**: "application/zip" (limited support)

**PROVIDER-SPECIFIC BEHAVIOR**
-   **Google Gemini**: Uses inline base64 data for smaller files, may use Files API for larger files
-   **Anthropic**: Supports inline base64 data with size limitations
-   **OpenAI-compatible**: Limited support depending on model capabilities

**FILE SIZE LIMITATIONS**
File size limits vary by provider and are subject to their API restrictions. Large files may be rejected or require alternative upload methods.

**RETURN VALUE**
Returns `true` on success, `false` on failure (e.g., memory allocation error, invalid parameters).

**EXAMPLE**
```c
// Read and encode a PDF file
FILE *file = fopen("document.pdf", "rb");
// ... read file content and base64 encode it ...

bool success = dp_message_add_file_data_part(
    &message, 
    "application/pdf", 
    base64_encoded_data, 
    "document.pdf"
);
if (!success) {
    fprintf(stderr, "Failed to add file attachment\n");
}
```

---
### Conversation Serialization Helpers
**SYNOPSIS**
```c
#include <disasterparty.h>
int dp_serialize_messages_to_json_str(const dp_message_t *messages, size_t num_messages, char **json_str_out);
int dp_deserialize_messages_from_json_str(const char *json_str, dp_message_t **messages_out, size_t *num_messages_out);
int dp_serialize_messages_to_file(const dp_message_t *messages, size_t num_messages, const char *path);
int dp_deserialize_messages_from_file(const char *path, dp_message_t **messages_out, size_t *num_messages_out);
```

**DESCRIPTION**
These functions provide utilities to save and load conversation histories.
-   **dp_serialize_...**: Convert an array of `dp_message_t` into a JSON string or save it directly to a file. The caller must `free()` the string returned by `..._to_json_str`.
-   **dp_deserialize_...**: Parse a JSON string or file content into a newly allocated array of `dp_message_t`. The caller is responsible for freeing the returned array and its contents using **dp_free_messages()**.
All functions return `0` on success and `-1` on failure.
