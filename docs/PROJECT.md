# Disaster Party LLM Client Library - Project Details
**Current Version:** 0.6.0
**Date:** 2026-03-13
**Primary Author:** Kirn Gill II <segin2005@gmail.com>
**Conceptualization & Initial Code Generation:** Gemini

## 1. Project Overview

The **Disaster Party** library provides a robust, portable C interface for interacting with Large Language Model (LLM) APIs, currently supporting OpenAI-compatible endpoints, the Google Gemini API, and the Anthropic Claude API.

### Core Features (v0.6.0)

* **Multi-Provider Support:** Unified interface for OpenAI, Google Gemini, and Anthropic.
* **Advanced Features:** Opt-in support for thinking/reasoning tokens via `DP_FEATURE_THINKING`.
* **Safe Streaming:** Automatic chunking of output tokens to protect consumers from buffer overflows.
* **Tool Calling:** Full support for function calling across all providers.
* **Multimodal Inputs:** Support for text, images, and file attachments.
* **Image Generation:** Support for DALL-E and Imagen.

### Architecture

The library is organized into focused modules:
- **`disasterparty.c`** - entry points
- **`dp_context.c`** - context and features
- **`dp_stream.c`** - SSE and chunking logic
- ... (see DOCUMENTATION.md for full list)

## 2. Public API Summary

### 2.1. Key Structures

#### `dp_request_config_t`
```c
typedef struct {
    const char* model;
    dp_message_t* messages;
    size_t num_messages;
    const char* system_prompt; 
    // ... generation params (temp, top_p, etc)
    const dp_tool_definition_t* tools;
    struct {
        bool enabled;
        int budget_tokens;
    } thinking;
} dp_request_config_t;
```

### 2.2. Key Functions

* `dp_enable_advanced_features(ctx, ...)` - Enable opt-in behaviors.
* `dp_perform_streaming_completion(...)` - Stream text tokens (safe-chunked).
* `dp_perform_detailed_streaming_completion(...)` - Cross-provider detailed events.

## 3. Build System

Standard Autotools: `./autogen.sh && ./configure && make && make check`.
Requires `libcurl` and `libcjson`.
