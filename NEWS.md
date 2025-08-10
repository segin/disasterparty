# Version 0.5.0 (2025-08-09)

## New Features and API Additions

* **ABI BREAKING CHANGE**: The `dp_content_part_type_t` enum has been extended with `DP_CONTENT_PART_FILE_DATA`. The `dp_content_part_t` struct has been extended with a `file_data` member. The `dp_context_s` struct has been extended with a `user_agent` field. SOVER incremented from 3:0:0 to 4:0:0 (libdisasterparty.so.4.0.0). Recompilation of applications will be required.

* **File Attachments Support**: Added support for general file attachments (PDFs, CSVs, text files, etc.) across all providers:
  * New `dp_message_add_file_data_part()` function for base64-encoded file attachments
  * New `dp_file_t` structure for file metadata
  * New `dp_upload_file()` and `dp_free_file()` functions for Gemini file uploads
  * New `dp_message_add_file_reference_part()` for referencing uploaded files

* **Custom User-Agent Support**: Added application identification in HTTP requests:
  * New `dp_init_context_with_app_info()` function for custom user-agent strings
  * Applications can now identify themselves in API requests

* **Token Counting**: Completed token counting functionality for Gemini and Anthropic providers:
  * New `dp_count_tokens()` function
  * New `dp_token_param_type_t` enum for parameter compatibility
  * Automatic `max_tokens` parameter compatibility across providers

## Major Code Restructuring

* **Modular Architecture**: Complete restructuring from monolithic `disasterparty.c` (3 files) to modular design (12 files):
  * `disasterparty.c` - Main library entry point and version information
  * `dp_constants.c` - Provider constants and default configurations  
  * `dp_context.c` - Context initialization and management
  * `dp_request.c` - Request handling and API communication
  * `dp_message.c` - Message construction and manipulation
  * `dp_stream.c` - Streaming response handling
  * `dp_serialize.c` - Message serialization/deserialization
  * `dp_file.c` - File upload and management
  * `dp_models.c` - Model listing functionality
  * `dp_utils.c` - Utility functions and helpers
  * `dp_private.h` - Internal function declarations

## Enhanced Test Coverage

* **25 New Test Files**: Comprehensive test suite expansion covering:
  * Authentication failure tests for all providers (`test_*_auth_failure_dp.c`)
  * Token counting tests (`test_*_token_counting_dp.c`, `test_*_multimodal_token_count_dp.c`)
  * File handling tests (`test_file_attachments_dp.c`, `test_upload_*_dp.c`)
  * Error handling tests (`test_*_error_dp.c`, `test_deserialize_malformed_file_dp.c`)
  * Rate limiting tests (`test_rate_limit_*_dp.c`)
  * Edge case tests (`test_abrupt_stream_dp.c`, `test_invalid_provider_dp.c`)
  * User-agent testing (`test_user_agent_dp.c`)

* **Test Infrastructure**: Added comprehensive testing support:
  * Mock server infrastructure with Python-based test servers
  * Test assets directory with various file types for testing
  * `TESTING.md` documentation for test procedures

## Build System and Quality Improvements

* **Strict Compilation**: All code now compiles cleanly with `-Wall -Werror`
* **Enhanced Build System**: Updated autotools configuration for modular structure
* **Manual Pages**: All 30 manual pages updated with current date (August 09, 2025)
* **Documentation**: Updated `DOCUMENTATION.md`, `api.json`, and `README.md` to reflect new architecture

## Repository Maintenance

* **Kiro Specifications**: Added comprehensive development specifications in `.kiro/specs/`
* **Project Documentation**: Enhanced `docs/PROJECT.md` and `docs/ROADMAP.md`
* **Repository Cleanup**: Removed abandoned development branches and artifacts

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