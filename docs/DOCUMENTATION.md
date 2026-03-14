# Disaster Party LLM Client Library - Full Documentation
**Version: 0.6.0**
**Last Updated: March 13, 2026**

## Code Structure

The Disaster Party library has been refactored into a modular structure for improved maintainability:

### Source Files
- **disasterparty.c** - Main library entry point and version information
- **dp_constants.c** - Provider constants and default configurations
- **dp_context.c** - Context initialization and management
- **dp_request.c** - Request handling and API communication
- **dp_message.c** - Message construction and manipulation
- **dp_stream.c** - Streaming response handling and safety chunking
- **dp_serialize.c** - Message serialization/deserialization
- **dp_file.c** - File upload and management
- **dp_models.c** - Model listing functionality
- **dp_utils.c** - Utility functions and helpers

### Header Files
- **disasterparty.h** - Public API declarations
- **dp_private.h** - Internal function declarations and shared structures

## 1. Overview (disasterparty.7)

### NAME
**disasterparty** - overview of the Disaster Party LLM client library

### DESCRIPTION
The **Disaster Party** library provides a C interface for interacting with Large Language Model (LLM) APIs, currently supporting OpenAI-compatible endpoints, the Google Gemini API, and the Anthropic Claude API. It aims to simplify common tasks such as sending text or multimodal prompts and handling both regular (blocking) and streaming responses.

Key features include:
- Support for text-only and multimodal (text + images + file attachments) inputs.
- Handling of both non-streaming and streaming completions.
- **Advanced Features**: Opt-in support for "Thinking" tokens and internal reasoning.
- **Consumer Protection**: Automated chunking of streaming tokens to prevent buffer overflows in legacy apps.
- Tool calling / Function calling support.
- Conversation serialization/deserialization.
- Image generation support.

### GETTING STARTED
1.  Initialize a context using **dp_init_context**(3).
2.  (Optional) Enable advanced features using **dp_enable_advanced_features**(3).
3.  Create messages using helper functions.
4.  Perform the API call using **dp_perform_completion**(3) or **dp_perform_streaming_completion**(3).
5.  Always free allocated resources.

---
## 2. API Functions (section 3)

### dp_enable_advanced_features
**NAME**
dp_enable_advanced_features - enable advanced library features

**SYNOPSIS**
```c
#include <disasterparty.h>
void dp_enable_advanced_features(dp_context_t *context, ...);
```

**DESCRIPTION**
Enables advanced feature flags for the given context. This function takes a variable number of features, terminated by 0.

**AVAILABLE FEATURES**
-   `DP_FEATURE_THINKING`: Enables processing of model reasoning/thought blocks (e.g., Gemini `thought`, OpenAI `reasoning_content`). By default, these are filtered out.

**EXAMPLE**
```c
dp_enable_advanced_features(ctx, DP_FEATURE_THINKING, 0);
```

---
### dp_perform_detailed_streaming_completion
**NAME**
dp_perform_detailed_streaming_completion - streaming request with detailed event handling

**SYNOPSIS**
```c
#include <disasterparty.h>
int dp_perform_detailed_streaming_completion(dp_context_t *context, const dp_request_config_t *request_config, dp_detailed_stream_callback_t callback, void *user_data, dp_response_t *response);
```

**DESCRIPTION**
Generalization of streaming completion that provides detailed events (e.g., start, delta, thinking, stop) across all supported providers.

---
### Message Construction Helpers
**SYNOPSIS**
```c
bool dp_message_add_text_part(dp_message_t *message, const char *text);
bool dp_message_add_image_url_part(dp_message_t *message, const char *image_url);
bool dp_message_add_base64_image_part(dp_message_t *message, const char *mime_type, const char *base64_data);
bool dp_message_add_file_data_part(dp_message_t *message, const char *mime_type, const char *base64_data, const char *filename);
bool dp_message_add_file_reference_part(dp_message_t *message, const char *file_id, const char *mime_type);
bool dp_message_add_tool_call_part(dp_message_t *message, const char *id, const char *function_name, const char *arguments_json);
bool dp_message_add_tool_result_part(dp_message_t *message, const char *tool_call_id, const char *content, bool is_error);
bool dp_message_add_thinking_part(dp_message_t *message, const char *thinking, const char *signature);
```

---
### dp_generate_image
**NAME**
dp_generate_image - generate images from text prompts

**SYNOPSIS**
```c
int dp_generate_image(dp_context_t* context, const dp_image_generation_config_t* config, dp_image_generation_response_t* response);
```
