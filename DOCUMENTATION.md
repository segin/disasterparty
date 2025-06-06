# Disaster Party LLM Client Library - Full Documentation
**Version: 0.2.0**

## 1. Overview (disasterparty.7)

### NAME
**disasterparty** - overview of the Disaster Party LLM client library

### DESCRIPTION
The **Disaster Party** library provides a C interface for interacting with Large Language Model (LLM) APIs, currently supporting OpenAI-compatible endpoints, the Google Gemini API, and the Anthropic Claude API. It aims to simplify common tasks such as sending text or multimodal prompts and handling both regular (blocking) and streaming responses.

The library is designed to be relatively simple to integrate and use, leveraging **libcurl**(3) for HTTP(S) communication and **cJSON**(3) for robust JSON parsing and generation.

Key features include:
- Support for text-only and multimodal (text + image URL/base64) inputs.
- Handling of both non-streaming (full response at once) and streaming (token-by-token) completions.
- A dedicated, detailed streaming callback for Anthropic's event-based stream.
- Interface for listing available models from the supported providers.
- Helper functions for constructing request messages and serializing/deserializing conversations.
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
1.  Initialize a context using **dp_init_context**(3), specifying the provider and your API key.
2.  Construct your request messages using `dp_message_t` and helper functions like **dp_message_add_text_part**(3).
3.  Configure your request using `dp_request_config_t`, setting the model, messages, temperature, etc.
4.  Perform the API call using either **dp_perform_completion**(3) for a blocking response or **dp_perform_streaming_completion**(3) for streaming. For Anthropic, you can use the detailed **dp_perform_anthropic_streaming_completion**(3).
5.  To list available models, use **dp_list_models**(3).
6.  To save/load conversations, use **dp_serialize_messages_to_file**(3) and **dp_deserialize_messages_from_file**(3).
7.  Always free allocated resources using functions like **dp_free_response_content**(3), **dp_free_model_list**(3), **dp_free_messages**(3), and finally **dp_destroy_context**(3).

### ENVIRONMENT
The test programs and typical applications using this library expect API keys to be set in environment variables:
-   **OPENAI_API_KEY**: For OpenAI-compatible services.
-   **GEMINI_API_KEY**: For Google Gemini services.
-   **ANTHROPIC_API_KEY**: For Anthropic Claude services.

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
**dp_init_context**(3), **dp_perform_completion**(3), **dp_list_models**(3), **curl**(1), **cJSON**(3)

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
```

**DESCRIPTION**
These helper functions add a new content part of the specified type to a `dp_message_t` structure, reallocating memory as needed. They return `true` on success and `false` on failure.

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
