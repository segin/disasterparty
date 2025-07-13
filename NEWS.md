# Version 0.5.0 (2025-07-12)

*   **File Uploads:** Implemented `dp_upload_file()` and `dp_free_file()` to support file uploads for the Google Gemini provider. This allows for a two-step, upload-then-reference workflow for multimodal prompts.
*   **Token Counting:** Added the `dp_count_tokens()` function to count tokens for a given request. This is supported for Google Gemini and Anthropic, and returns `DP_ERROR_TOKEN_COUNTING_NOT_SUPPORTED` for OpenAI.
*   **Custom User-Agent:** Added `dp_set_user_agent()` and `dp_get_user_agent()` to allow applications to prepend their own name and version to the HTTP User-Agent string for better API tracking.
*   **New Error Code:** Added the `DP_ERROR_TOKEN_COUNTING_NOT_SUPPORTED` error code to `dp_error_code_t`.
*   **Documentation:** Updated all man pages, `DOCUMENTATION.md`, and `api.json` to reflect the new features and API changes. Added new man pages for all new public functions and data types.

# Version 0.4.0 (2025-07-03)
* **ABI BREAKING CHANGE**: The `dp_content_part_t` struct has been modified with the addition of a `file_uri` member. The `dp_content_part_type_t` enum has also been updated with `DP_CONTENT_PART_FILE_REFERENCE`. Recompilation of applications will be required.
* Added support for file uploads and file references in messages (Gemini only).
* Updated Gemini default models in tests to `gemini-2.5-flash`.
* Improved test robustness and reporting, treating API quota errors (HTTP 429) as skips.
* Updated all manual pages with correct dates and properly formatted example code.
* Updated `README.md` and `api.json` to reflect new features and version.

# Version 0.3.0 (2025-06-14)
* **ABI BREAKING CHANGE**: The `dp_request_config_t` struct will be modified. Recompilation of applications will be required.
* Add support for expanded generation parameters: `top_p`, `top_k`, and `stop_sequences`.
* Implement dedicated `system_instruction` support for the Gemini API.
* Add new unit tests for error handling and advanced parameter submission.

# Version 0.2.1 (2025-06-08)
* Added C++ compatibility guards (`extern "C"`) to the public header file.
* Added conversation serialization/deserialization helper functions (to/from JSON string and file).
* Fixed a streaming bug that could cause premature write errors.
* Added comprehensive unit tests for Anthropic API functionality and serialization.
* Updated all documentation and man pages to reflect the current API and version.

# Version 0.2.0 (2025-06-07)
* Added support for Anthropic Claude API.
  * New provider type `DP_PROVIDER_ANTHROPIC`.
  * Implemented `dp_perform_completion` for Anthropic.
  * Implemented `dp_perform_streaming_completion` (generic text callback) for Anthropic.
  * Added `dp_perform_anthropic_streaming_completion` with a new `dp_anthropic_stream_callback_t` for detailed Anthropic SSE event handling (`dp_anthropic_stream_event_t`).
  * Added `system_prompt` to `dp_request_config_t`.
  * Updated `dp_list_models` to support Anthropic.
* Updated library version to 0.2.0 across all relevant files.
* User-Agent string updated to `disasterparty/0.2.0`.

# Version 0.1.1 (2025-06-02)
* Updated library version to 0.1.1.
* Corrected SSE event separator detection for Gemini streams (handles `\r\n\r\n`).
* Updated OpenAI test model from `gpt-3.5-turbo` to `gpt-4.1-nano`.
* Updated Gemini test model to `gemini-2.0-flash` and token limits.
* Added `dp_list_models()` and `dp_free_model_list()` functions for enumerating models available at the API endpoint.
* Added man pages for all public functions and a library overview (section 7).
* Updated User-Agent string to `disasterparty/0.1.1`.
* Standardized bug reporting in man pages to GitHub issues.
* Minor internal fixes and debugging log cleanup.

# Version 0.1.0 (2025-05-01)
* Initial release of the Disaster Party LLM client library.
* Supports OpenAI-compatible and Google Gemini APIs.
* Features:
  * Text and multimodal (text + image URL/base64) inputs.
  * Non-streaming (full response) and streaming (token-by-token) completions.
  * Uses cJSON for robust JSON handling.
  * Uses libcurl for HTTP communication.
* Build system based on GNU Autotools.
* Renamed from an earlier prototype to "Disaster Party".