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

### 2.2. Implement Token Counting

* **Goal:** Provide a way for library consumers to accurately count the number of tokens in a prompt before sending it, to manage costs and context windows.
* **Difficulty:** Medium (due to differing provider strategies)
* **Implementation Plan:**
    1.  **New API Function:** Add `int dp_count_tokens(dp_context_t* context, const dp_request_config_t* request_config, size_t* token_count_out);`.
    2.  **Network-Based (Gemini/Anthropic):** Implement the function to call their respective `countTokens` API endpoints.
    3.  **Client-Side (OpenAI):** Implement an optional feature, enabled by a `./configure` flag (`--enable-tiktoken`), that links against a C-compatible `tiktoken` library (e.g., `kojix2/tiktoken-c`) to provide fast, local, and accurate token counts for OpenAI models.
    4.  **New Unit Test:** Create `test_token_counting_dp.c` to verify the functionality.

---

## 3. Long-Term Goals / Major Architectural Additions

These features represent significant new capabilities and would likely require a major version bump.

### 3.1. Implement Tool Calls (Function Calling)

* **Goal:** Allow the library to serve as the engine for agentic workflows by enabling the model to request the execution of application-defined functions.
* **Difficulty:** High
* **What it would take:** New data structures for tool definitions (`dp_tool_definition_t`) and tool call requests (`dp_tool_call_t`); adding a `tools` array to `dp_request_config_t`; and significantly enhancing response parsing to handle the `tool_calls` finish reason.

### 3.2. Access to "Thinking" Tokens & Metacognition

* **Goal:** Expose the reasoning process of models that support it, primarily Anthropic's Claude.
* **Difficulty:** High
* **Implementation Steps:** Enhance the `dp_anthropic_stream_callback_t` to handle `thinking` events from the Anthropic API and update the internal stream parser accordingly.

### 3.3. Support for New Modalities

* **Image Generation (DALL-E):** Requires a new function (`dp_generate_image`) and request/response structs for the `/v1/images/generations` endpoint.
* **Speech-to-Text (Whisper):** High difficulty due to the need for `multipart/form-data` uploads. Requires a new function (`dp_transcribe_audio`).
* **Text-to-Speech (TTS):** Medium difficulty. Requires a new function (`dp_generate_speech`) and logic to handle a binary audio stream as the response.
* **Real-Time Conversational Audio:** Very high difficulty. Requires a new networking backend (e.g., WebSockets) and complex asynchronous I/O management.
* **Text-to-Video (Veo):** High difficulty. Requires managing an asynchronous job submission and polling workflow with multiple new functions.

### 3.4. Support for Interactive Playgrounds & Code Canvases

* **Goal:** Enable `libdisasterparty` to be the backend for complex, interactive applications.
* **Note:** This is an **application-level** goal. The library's role is to provide the necessary low-level capabilities.
* **Prerequisites from `libdisasterparty`:**
    1.  **Tool Calling:** This is the most critical prerequisite.
    2.  **Conversation Serialization:** The existing serialization functions are essential for saving and loading state. (complete)