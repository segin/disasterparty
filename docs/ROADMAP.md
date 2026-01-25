# Disaster Party - Comprehensive Project Roadmap

This document outlines potential new features and enhancements for the `libdisasterparty` library, combining all previously discussed planning items into a single roadmap.

---

## 1. Near-Term Priorities / Low-Hanging Fruit (complete)

These features were relatively straightforward to implement and built directly upon the existing architecture.

### 1.1. Full Support for Gemini System Prompts (complete)

* **Status:** The library's `dp_request_config_t` has a `system_prompt` field, which is now correctly used by the payload builders for all three providers, including Gemini's `system_instruction` format.

### 1.2. Expanded Model Parameters (complete)

* **Status:** The library now supports `top_p`, `top_k`, and `stop_sequences` in addition to `temperature` and `max_tokens`.

---

## 2. Medium-Term Goals / Core Feature Enhancements

These features enhance the core capabilities of the library.

### 2.1. Advanced Multimodality

* **Goal:** Formally test and document support for multiple images, inline context (e.g., Text -> Image -> Text), and general file attachments.

#### 2.1.1. Multiple Images & Inline Context (complete)
* **Status:** The library's `parts` array design supports this, and a dedicated unit test (`test_inline_multimodal_dp.c`) has been created to verify the construction of interleaved messages.

#### 2.1.2. General File Attachments (PDFs, CSVs, etc.)
* **Difficulty:** Medium
* **Action Items:**
    1.  **API Investigation:** Confirm the exact JSON structure for non-image file data for both Gemini and Anthropic. This will likely involve a two-step upload-then-reference workflow using their respective Files APIs.
    2.  **(Optional) Header Update:** Consider adding a `DP_CONTENT_PART_FILE_DATA` enum value to `dp_content_part_type_t` for code clarity.
    3.  **New Helper Function:** Create a new convenience function, e.g., `dp_message_add_file_data_part()`.
    4.  **Payload Builder Updates:** Update the Gemini and Anthropic payload builders to handle this new part type.
    5.  **New Unit Test:** Add a test case that sends a base64-encoded text file or PDF.

## [2.2] - 2024-03-21

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

## [2.3] - 2025-07-11

* **Goal:** Provide a way for library consumers to accurately count the number of tokens in a prompt before sending it, to manage costs and context windows.
* **Difficulty:** Medium (due to differing provider strategies)
* **Implementation Plan:**
    1.  **New API Function:** Add `int dp_count_tokens(dp_context_t* context, const dp_request_config_t* request_config, size_t* token_count_out);`.
    2.  **Network-Based (Gemini/Anthropic):** Implement the function to call their respective `countTokens` API endpoints.
    3.  **OpenAI:** Token counting is not supported for the OpenAI provider. The `dp_count_tokens` function will return an error for this provider.
    4.  **New Unit Test:** Create `test_token_counting_dp.c` to verify the functionality.

---

## 3. Long-Term Goals / Major Architectural Additions

These features represent significant new capabilities and would likely require a major version bump.

### 3.1. Implement Tool Calls (Function Calling) & Interactive Application Support (complete)

* **Status:** Implemented in version 0.6.0. Includes `dp_tool_definition_t`, `dp_tool_choice_t`, support in `dp_request_config_t`, and full payload construction/response parsing for OpenAI, Gemini, and Anthropic.
* **Note:** This feature satisfies the requirements for `libdisasterparty` to serve as the backend for complex, interactive applications (Playgrounds, Code Canvases), alongside the existing conversation serialization capabilities.

### 3.2. Access to "Thinking" Tokens & Metacognition (complete)

* **Status:** Implemented in version 0.6.0. Added `DP_CONTENT_PART_THINKING`, `dp_message_add_thinking_part`, and streaming support via `DP_ANTHROPIC_EVENT_THINKING_DELTA`.

---

## 4. Support for New Modalities

This section outlines the expansion of `libdisasterparty` to support modalities beyond text and static images.

### 4.1. Image Generation (e.g., DALL-E)

* **Goal:** Enable users to generate images from text prompts.
* **Difficulty:** Medium
* **Action Items:**
    1.  **Struct Definition:** Define `dp_image_generation_config_t` (prompt, size, quality, style) and `dp_image_generation_response_t` (url or base64 data).
    2.  **API Implementation:** Implement `dp_generate_image` function targeting the `/v1/images/generations` endpoint (OpenAI) and equivalent for Google/Anthropic if available.
    3.  **Testing:** Create `test_image_generation_dp.c`.

### 4.2. Speech-to-Text (e.g., Whisper)

* **Goal:** Transcribe audio files into text.
* **Difficulty:** High
* **Action Items:**
    1.  **Multipart Support:** Implement `multipart/form-data` encoding in `dp_request.c` (currently we mostly do JSON).
    2.  **API Implementation:** Implement `dp_transcribe_audio` function accepting a file path or buffer.
    3.  **Testing:** Create `test_audio_transcription_dp.c` with sample audio files.

### 4.3. Text-to-Speech (TTS)

* **Goal:** Convert text into spoken audio.
* **Difficulty:** Medium
* **Action Items:**
    1.  **Binary Response Handling:** Enhance response processing to handle binary audio streams instead of JSON.
    2.  **API Implementation:** Implement `dp_generate_speech` function.
    3.  **Testing:** Create `test_text_to_speech_dp.c` verifying audio headers/format.

### 4.4. Text-to-Video (e.g., Veo, Sora)

* **Goal:** Generate videos from text prompts.
* **Difficulty:** High
* **Action Items:**
    1.  **Async Workflow:** Design a polling mechanism for long-running jobs, as video generation is rarely synchronous.
    2.  **API Implementation:** Implement `dp_submit_video_job` and `dp_get_video_job_status`.
    3.  **Testing:** Create `test_video_generation_dp.c`.

### 4.5. Real-Time Conversational Audio

* **Goal:** Low-latency, bidirectional audio streaming.
* **Difficulty:** Very High
* **Action Items:**
    1.  **WebSocket Backend:** Research and integrate a WebSocket library (e.g., libwebsockets or curl's WS support) into the context manager.
    2.  **Event Loop:** Design an asynchronous event loop for handling incoming audio chunks and events.
    3.  **API Implementation:** Implement `dp_start_realtime_session` and callback handlers.