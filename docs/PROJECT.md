# Disaster Party LLM Client Library - Full Documentation
**Current Version:** 0.3.0
**Date:** 2025-06-14
**Primary Author:** Kirn Gill II <segin2005@gmail.com>
**Conceptualization & Initial Code Generation:** Gemini

## 1. Project Overview

The **Disaster Party** library provides a robust, portable C interface for interacting with Large Language Model (LLM) APIs, currently supporting OpenAI-compatible endpoints, the Google Gemini API, and the Anthropic Claude API. It aims to simplify common tasks such as sending text or multimodal prompts and handling both regular (blocking) and streaming responses.

The library is designed to be relatively simple to integrate and use, leveraging **libcurl**(3) for HTTP(S) communication and **cJSON**(3) for robust JSON parsing and generation.

### Core Features (as of v0.3.0)

* **Multi-Provider Support:** A unified interface for interacting with OpenAI, Google Gemini, and Anthropic.
* **Multimodal Inputs:** Support for sending interleaved text and image data (via URL for OpenAI, and base64 for Gemini/Anthropic) in a single prompt.
* **Granular Request Control:** Support for advanced generation parameters, including `temperature`, `max_tokens`, `top_p`, `top_k`, and custom `stop_sequences`.
* **Handling of both non-streaming (full response at once) and streaming (token-by-token) completions.
* **A dedicated, detailed streaming callback for Anthropic's event-based stream.
* **Interface for listing available models from the supported providers.
* **Helper functions for constructing request messages and serializing/deserializing conversations to/from JSON or files.
* **Robust Build System:** A standard GNU Autotools setup (`./configure && make && make install`) that correctly handles dependencies.
* **Comprehensive Test Suite:** A suite of integration tests that verify functionality against live APIs and can be skipped if API keys are not available.

### SYNOPSIS
To use the Disaster Party library in your C program, include the main header file:
```c
#include <disasterparty.h>
```
When compiling and linking, you will typically need to link against `libdisasterparty`, `libcurl`, and `libcjson`. Using pkg-config is recommended if available:
```sh
gcc my_app.c $(pkg-config --cflags --libs disasterparty)
```

## 2. Public API Documentation

This section details every public data structure and function available in `disasterparty.h`.

### 2.1. Core Data Structures

#### `dp_context_t`
This is an opaque handle that represents a configured session with a specific LLM provider. It is created by `dp_init_context()` and must be destroyed with `dp_destroy_context()`.

#### `dp_request_config_t`
This structure holds all parameters for an LLM request. It must be initialized (e.g., `dp_request_config_t config = {0};`) before use.
```c
typedef struct {
    const char* model;
    dp_message_t* messages;
    size_t num_messages;
    const char* system_prompt; 
    double temperature;       
    int max_tokens;           
    bool stream;              
    double top_p;
    int top_k;
    const char** stop_sequences;
    size_t num_stop_sequences;
} dp_request_config_t;
```
- **`model`**: The ID of the model to use (e.g., "gpt-4.1-nano", "gemini-2.0-flash").
- **`messages`**: An array of `dp_message_t` structures forming the conversation history.
- **`num_messages`**: The number of messages in the array.
- **`system_prompt`**: An optional system prompt to influence the model's behavior. This is handled correctly for each provider's specific implementation (system message, top-level system field, or system instruction).
- **`temperature`**: Controls randomness (e.g., 0.0 to 1.0).
- **`max_tokens`**: The maximum number of tokens to generate.
- **`stream`**: If `true`, the request will be made in streaming mode.
- **`top_p`**: Nucleus sampling parameter.
- **`top_k`**: Top-k sampling parameter.
- **`stop_sequences`**: An array of strings that will cause generation to stop.
- **`num_stop_sequences`**: The number of strings in the `stop_sequences` array.

#### `dp_response_t`
This structure holds the result of a non-streaming API call or the final status of a streaming call. Its contents must be freed with `dp_free_response_content()`.
```c
typedef struct {
    dp_response_part_t* parts; 
    size_t num_parts;          
    char* error_message;        
    long http_status_code;      
    char* finish_reason;      
} dp_response_t;
```
- **`parts`**: For non-streaming responses, this is an array containing the response content. `parts[0].text` holds the primary text output.
- **`num_parts`**: The number of parts in the array.
- **`error_message`**: A string describing any transport-level or API error that occurred. `NULL` on success.
- **`http_status_code`**: The HTTP status code from the server (e.g., 200, 401, 503).
- **`finish_reason`**: A string from the API indicating why generation stopped (e.g., "stop", "length", "end_turn").

#### `dp_message_t` and `dp_content_part_t`
These structures are used to build the conversation history, allowing for complex, multimodal messages with interleaved text and images.
```c
typedef struct {
    dp_message_role_t role;
    dp_content_part_t* parts;
    size_t num_parts;
} dp_message_t;

typedef struct {
    dp_content_part_type_t type;
    char* text;
    char* image_url;
    struct {
        char* mime_type;
        char* data; 
    } image_base64;
} dp_content_part_t;
```

#### `dp_model_list_t` and `dp_model_info_t`
These structures are used by `dp_list_models()` to return information about available models.
```c
typedef struct {
    dp_model_info_t* models;    
    size_t count;               
    char* error_message;        
    long http_status_code;      
} dp_model_list_t;

typedef struct {
    char* model_id;         
    char* display_name;     
    char* version;          
    char* description;      
    long input_token_limit; 
    long output_token_limit;
} dp_model_info_t;
```

#### Anthropic-Specific Streaming Structures
```c
typedef enum {
    DP_ANTHROPIC_EVENT_UNKNOWN,
    DP_ANTHROPIC_EVENT_MESSAGE_START,
    // ... other event types
    DP_ANTHROPIC_EVENT_ERROR
} dp_anthropic_event_type_t;

typedef struct {
    dp_anthropic_event_type_t event_type; 
    const char* raw_json_data;            
} dp_anthropic_stream_event_t;
```

### 2.2. Public Functions

#### Setup and Teardown
* `const char *dp_get_version(void);`
* `dp_context_t *dp_init_context(dp_provider_type_t provider, const char *api_key, const char *api_base_url);`
* `void dp_destroy_context(dp_context_t *context);`

#### Core API Calls
* `int dp_perform_completion(dp_context_t *context, const dp_request_config_t *request_config, dp_response_t *response);`
* `int dp_perform_streaming_completion(dp_context_t *context, const dp_request_config_t *request_config, dp_stream_callback_t callback, void *user_data, dp_response_t *response);`
* `int dp_perform_anthropic_streaming_completion(dp_context_t *context, const dp_request_config_t *request_config, dp_anthropic_stream_callback_t anthropic_callback, void *user_data, dp_response_t *response);`
* `int dp_list_models(dp_context_t *context, dp_model_list_t **model_list_out);`

#### Memory Management
* `void dp_free_response_content(dp_response_t *response);`
* `void dp_free_model_list(dp_model_list_t *model_list);`
* `void dp_free_messages(dp_message_t *messages, size_t num_messages);`

#### Message Construction
* `bool dp_message_add_text_part(dp_message_t *message, const char *text);`
* `bool dp_message_add_image_url_part(dp_message_t *message, const char *image_url);`
* `bool dp_message_add_base64_image_part(dp_message_t *message, const char *mime_type, const char *base64_data);`

#### Conversation Serialization
* `int dp_serialize_messages_to_json_str(const dp_message_t *messages, size_t num_messages, char **json_str_out);`
* `int dp_deserialize_messages_from_json_str(const char *json_str, dp_message_t **messages_out, size_t *num_messages_out);`
* `int dp_serialize_messages_to_file(const dp_message_t *messages, size_t num_messages, const char *path);`
* `int dp_deserialize_messages_from_file(const char *path, dp_message_t **messages_out, size_t *num_messages_out);`

---

## 3. Build System and Installation

The project uses the GNU Autotools suite for maximum portability.

### Dependencies
* `libcurl` (>= 7.20.0)
* `libcjson` (>= 1.7.10)
* A C99-compliant C compiler (e.g., GCC, Clang)
* Standard build tools: `make`, `pkg-config`, `autoconf`, `automake`, `libtool`

### Build Process
The standard build process is followed:
1.  `./autogen.sh` - Generates the `configure` script and `Makefile.in` files.
2.  `./configure` - Checks for dependencies and creates the Makefiles.
3.  `make` - Compiles the library and test programs.
4.  `make check` - Runs the test suite.
5.  `sudo make install` - Installs the library, headers, and man pages.

---

## 4. Debugging History & Key Fixes

Several key issues were identified and fixed during development:

1.  **Gemini Streaming Failure:**
    * **Symptom:** The Gemini streaming test completed with HTTP 200 but produced no tokens.
    * **Debug Process:** By using `curl` directly and then adding verbose `fprintf(stderr, ...)` logs to the library's `streaming_write_callback`, we discovered that the Gemini SSE stream was using `\r\n\r\n` as the event separator.
    * **Fix:** The `streaming_write_callback` function in `disasterparty.c` was modified to search for both `\n\n` and `\r\n\r\n` to correctly parse SSE events regardless of the line ending convention.

2.  **`make check` False Failures:**
    * **Symptom:** Tests that were successful when run directly from the shell would be marked as `FAIL` by `make check`.
    * **Debug Process:** We identified that the tests were exiting with status `1` despite appearing successful. The root cause was determined to be the test harness interpreting any output to `stderr` as a failure.
    * **Fix:** All debugging `fprintf` statements were removed from the final versions of the test files, ensuring they only produce output on `stderr` in a true error condition.

3.  **Autotools Portability Issues:**
    * **Symptom:** `automake` would fail with warnings about non-POSIX variable names in the `uninstall-hook` of `man/Makefile.am`.
    * **Fix:** The `uninstall-hook` was rewritten to use a standard POSIX `for` loop instead of the GNU Make-specific `$(addprefix ...)` function, ensuring portability.

---

## 5. Project Roadmap

### 5.1. Near-Term Priorities / Low-Hanging Fruit (complete)

These features were relatively straightforward to implement and built directly upon the existing architecture.

#### 5.1.1. Full Support for Gemini System Prompts (complete)
- **Status:** The library's `dp_request_config_t` has a `system_prompt` field, which is now correctly used by the payload builders for all three providers, including Gemini's `system_instruction` format.

#### 5.1.2. Expanded Model Parameters (complete)
- **Status:** The library now supports `top_p`, `top_k`, and `stop_sequences` in addition to `temperature` and `max_tokens`.

### 5.2. Medium-Term Goals / Core Feature Enhancements

These features enhance the core capabilities of the library.

#### 5.2.1. Advanced Multimodality
* **Multiple Images & Inline Context (complete):** The library's `parts` array design supports this, and a dedicated unit test (`test_inline_multimodal_dp.c`) verifies the construction of interleaved messages.
* **General File Attachments (PDFs, CSVs, etc.):** This is the next logical step. It requires implementing a `multipart/form-data` upload mechanism (likely a new internal helper function) and adding a new content part type (e.g., `DP_CONTENT_PART_DOCUMENT_REFERENCE`) to handle the file IDs/URIs returned by the provider's Files API.

#### 5.2.2. Implement Token Counting
* **Goal:** Provide a way for library consumers to accurately count the number of tokens in a prompt before sending it, to manage costs and context windows.
* **Difficulty:** Medium (due to differing provider strategies)
* **Implementation Plan:**
    1.  **New API Function:** Add `int dp_count_tokens(dp_context_t* context, const dp_request_config_t* request_config, size_t* token_count_out);`.
    2.  **Network-Based (Gemini/Anthropic):** Implement the function to call their respective `countTokens` API endpoints.
    3.  **Client-Side (OpenAI):** Since OpenAI does not have a `countTokens` endpoint, this will require a different approach:
        * **Option A (Simpler):** The function can return an error or a rough estimate (e.g., based on character count), with documentation explaining the limitation.
        * **Option B (More Complex):** Find and integrate a C-based port of OpenAI's `tiktoken` library. This would add a new dependency and require an optional `--enable-tiktoken` configure flag.
    4.  **New Unit Test:** Create `test_token_counting_dp.c` to verify the functionality.

#### 5.2.3. Configurable User-Agent
* **Goal:** Allow applications using `libdisasterparty` to prepend their own name and version to the HTTP User-Agent string for better tracking and identification by API providers. The final string would be in the format `AppName/AppVersion (disasterparty/DP_VERSION)`.
* **Difficulty:** Low
* **Implementation Plan:**
    1.  **API Change:** Modify the `dp_init_context` function signature to accept two new optional parameters: `const char* app_name` and `const char* app_version`.
        ```c
        // New proposed signature
        dp_context_t *dp_init_context(dp_provider_type_t provider, 
                                      const char *api_key, 
                                      const char *api_base_url,
                                      const char *app_name,
                                      const char *app_version);
        ```
    2.  **Internal Change:** The `dp_context_s` struct will store the fully-formed user-agent string.
    3.  **Logic Update:** `dp_init_context` will construct the user-agent string. If `app_name` is `NULL`, it will default to `disasterparty/DP_VERSION`.
    4.  **Usage Update:** All `curl_easy_setopt` calls for `CURLOPT_USERAGENT` will use the string stored in the context.

### 5.3. Long-Term Goals / Major Architectural Additions

These features represent significant new capabilities and would likely require a major version bump.

#### 5.3.1. Implement Tool Calls (Function Calling)
* **Goal:** Allow the library to serve as the engine for agentic workflows by enabling the model to request the execution of application-defined functions.
* **Difficulty:** High
* **What it would take:** New data structures for tool definitions (`dp_tool_definition_t`) and tool call requests (`dp_tool_call_t`); adding a `tools` array to `dp_request_config_t`; and significantly enhancing response parsing to handle the `tool_calls` finish reason.

#### 5.3.2. Access to "Thinking" Tokens & Metacognition
* **Goal:** Expose the reasoning process of models that support it, primarily Anthropic's Claude.
* **Difficulty:** High
* **Implementation Steps:**
    1.  Focus on the Anthropic provider first, as it has the most explicit support for this feature.
    2.  Enhance the `dp_anthropic_stream_callback_t` and its underlying `dp_anthropic_stream_event_t` struct to handle `thinking` and `signature_delta` event types.
    3.  Update the `anthropic_detailed_stream_write_callback` in `disasterparty.c` to parse these new events and pass them to the user's callback.

#### 5.3.3. Support for New Modalities
* **Image Generation (DALL-E):** Requires a new function (`dp_generate_image`) and request/response structs for the `/v1/images/generations` endpoint.
* **Speech-to-Text (Whisper):** High difficulty due to the need for `multipart/form-data` uploads. Requires a new function (`dp_transcribe_audio`).
* **Text-to-Speech (TTS):** Medium difficulty. Requires a new function (`dp_generate_speech`) and logic to handle a binary audio stream as the response.
* **Real-Time Conversational Audio:** Very high difficulty. Requires a new networking backend (e.g., WebSockets) and complex asynchronous I/O management.
* **Text-to-Video (Veo):** High difficulty. Requires managing an asynchronous job submission and polling workflow with multiple new functions.
