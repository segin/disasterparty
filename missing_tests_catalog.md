# Missing Tests Catalog from test-expansions Branch

This document catalogs all test files that exist in the test-expansions branch but are missing from the main branch. Each test has been analyzed to understand its purpose and coverage.

## Summary

**Total Missing Tests:** 18 (3 migrated, 15 remaining)

The missing tests fall into the following categories:
- **Authentication Failure Tests:** 3 tests
- **Streaming Error Handling Tests:** 3 tests  
- **Token Counting Tests:** 2 tests
- **File Upload Tests:** 3 tests
- **Serialization Tests:** 2 tests
- **Rate Limiting Tests:** 2 tests
- **Edge Case/Error Handling Tests:** 3 tests

## Detailed Test Analysis

### Authentication Failure Tests

#### 1. `test_anthropic_auth_failure_dp.c`
- **Purpose:** Tests authentication failure handling for Anthropic API (HTTP 401)
- **Coverage:** Tests `dp_perform_completion`, `dp_list_models`, and `dp_count_tokens` with invalid API key
- **Mock Server:** Uses `DP_MOCK_SERVER` environment variable with special API key `AUTH_FAILURE_ANTHROPIC`
- **Expected Behavior:** All functions should fail with HTTP 401 status code
- **Requirements:** 2.1, 2.2

#### 2. `test_gemini_auth_failure_dp.c`
- **Purpose:** Tests authentication failure handling for Google Gemini API (HTTP 401)
- **Coverage:** Tests `dp_perform_completion`, `dp_list_models`, `dp_upload_file`, and `dp_count_tokens` with invalid API key
- **Mock Server:** Uses `DP_MOCK_SERVER` environment variable with special API key `AUTH_FAILURE_GEMINI`
- **Expected Behavior:** All functions should fail with HTTP 401 status code
- **Requirements:** 2.1, 2.2

#### 3. `test_openai_auth_failure_dp.c`
- **Purpose:** Tests authentication failure handling for OpenAI API (HTTP 401)
- **Coverage:** Tests `dp_perform_completion`, `dp_list_models`, and `dp_count_tokens` with invalid API key
- **Mock Server:** Uses `DP_MOCK_SERVER` environment variable with special API key `AUTH_FAILURE_OPENAI`
- **Expected Behavior:** All functions should fail with HTTP 401 status code
- **Requirements:** 2.1, 2.2

### Streaming Error Handling Tests

#### 4. `test_abrupt_stream_dp.c` ✅ **MIGRATED**
- **Purpose:** Tests handling of abruptly terminated streaming connections
- **Coverage:** Tests `dp_perform_streaming_completion` with connection that terminates unexpectedly
- **Mock Server:** Uses `DP_MOCK_SERVER` environment variable with special API key `ABRUPT_STREAM`
- **Expected Behavior:** Stream handler should receive error callback and function should fail gracefully
- **Migration Status:** Successfully adapted to main branch architecture and integrated with testsuite
- **Requirements:** 2.1, 2.2

#### 5. `test_anthropic_streaming_error_dp.c` ✅ **MIGRATED**
- **Purpose:** Tests Anthropic-specific streaming error event handling mid-stream
- **Coverage:** Tests `dp_perform_anthropic_streaming_completion` with mid-stream error events
- **Mock Server:** Uses `DP_MOCK_SERVER` environment variable with special API key `STREAM_ERROR_ANTHROPIC`
- **Expected Behavior:** Should receive message_start, content_block_start, first delta, then error event
- **Dependencies:** Requires cJSON library for JSON parsing
- **Migration Status:** Successfully adapted to main branch architecture and integrated with testsuite
- **Requirements:** 2.1, 2.2

#### 6. `test_anthropic_streaming_ping_dp.c` ✅ **MIGRATED**
- **Purpose:** Tests Anthropic-specific streaming ping event handling
- **Coverage:** Tests `dp_perform_anthropic_streaming_completion` with ping events interspersed in stream
- **Mock Server:** Uses `DP_MOCK_SERVER` environment variable with special API key `STREAM_PING_ANTHROPIC`
- **Expected Behavior:** Should properly handle ping events without affecting content processing
- **Dependencies:** Requires cJSON library for JSON parsing
- **Migration Status:** Successfully adapted to main branch architecture and integrated with testsuite
- **Requirements:** 2.1, 2.2

### Token Counting Tests

#### 7. `test_anthropic_multimodal_token_count_dp.c`
- **Purpose:** Tests token counting for multimodal messages with Anthropic API
- **Coverage:** Tests `dp_count_tokens` with text and multiple image URL parts
- **Test Data:** Uses dummy API key and claude-3-haiku-20240307 model
- **Expected Behavior:** Should fail with dummy API key (tests error handling)
- **Requirements:** 2.1, 2.2

#### 8. `test_gemini_multimodal_token_count_dp.c`
- **Purpose:** Tests token counting for multimodal messages with Gemini API
- **Coverage:** Tests `dp_count_tokens` with text and multiple image URL parts
- **Test Data:** Uses dummy API key and gemini-1.5-flash model
- **Expected Behavior:** Should fail with dummy API key (tests error handling)
- **Requirements:** 2.1, 2.2

### File Upload Tests

#### 9. `test_unsupported_file_uploads_dp.c`
- **Purpose:** Tests that file uploads fail appropriately for providers that don't support them
- **Coverage:** Tests `dp_upload_file` with OpenAI and Anthropic providers
- **Test Data:** Creates temporary file using mkstemp
- **Expected Behavior:** Should fail for both OpenAI and Anthropic providers
- **Requirements:** 2.1, 2.2

#### 10. `test_upload_large_file_dp.c`
- **Purpose:** Tests file upload behavior with very large files (>100MB)
- **Coverage:** Tests `dp_upload_file` with 101MB file using Gemini provider
- **Mock Server:** Uses `DP_MOCK_SERVER` environment variable with special API key `LARGE_FILE_UPLOAD`
- **Test Data:** Creates 101MB binary file filled with 'A' characters
- **Expected Behavior:** Should fail due to file size limits
- **Requirements:** 2.1, 2.2

#### 11. `test_upload_zero_byte_file_dp.c`
- **Purpose:** Tests file upload behavior with zero-byte files
- **Coverage:** Tests `dp_upload_file` with empty file using Gemini provider
- **Mock Server:** Uses `DP_MOCK_SERVER` environment variable with special API key `ZERO_BYTE_FILE`
- **Test Data:** Creates empty file
- **Expected Behavior:** Should fail due to empty file validation
- **Requirements:** 2.1, 2.2

### Serialization Tests

#### 12. `test_deserialize_malformed_file_dp.c`
- **Purpose:** Tests deserialization error handling with malformed JSON files
- **Coverage:** Tests `dp_deserialize_messages_from_file` with invalid JSON
- **Test Data:** Creates file with malformed JSON (missing braces/brackets)
- **Expected Behavior:** Should fail gracefully and return NULL messages
- **Requirements:** 2.1, 2.2

#### 13. `test_serialization_all_parts_dp.c`
- **Purpose:** Tests serialization/deserialization with all content part types
- **Coverage:** Tests round-trip serialization with text, image URL, base64 image, and file reference parts
- **Test Data:** Creates messages with all supported content part types
- **Expected Behavior:** Deserialized messages should exactly match original messages
- **Dependencies:** Includes comprehensive message comparison logic
- **Requirements:** 2.1, 2.2

### Rate Limiting Tests

#### 14. `test_rate_limit_completion_dp.c`
- **Purpose:** Tests API rate limiting handling for completion requests (HTTP 429)
- **Coverage:** Tests `dp_perform_completion` with rate-limited API key
- **Mock Server:** Uses `DP_MOCK_SERVER` environment variable with special API key `RATE_LIMIT_COMPLETION`
- **Expected Behavior:** Should fail with HTTP 429 status code and appropriate error message
- **Requirements:** 2.1, 2.2

#### 15. `test_rate_limit_list_models_dp.c`
- **Purpose:** Tests API rate limiting handling for model listing requests (HTTP 429)
- **Coverage:** Tests `dp_list_models` with rate-limited API key
- **Mock Server:** Uses `DP_MOCK_SERVER` environment variable with special API key `RATE_LIMIT_LIST_MODELS`
- **Expected Behavior:** Should fail with HTTP 429 status code and appropriate error message
- **Requirements:** 2.1, 2.2

### Edge Case/Error Handling Tests

#### 16. `test_invalid_provider_dp.c`
- **Purpose:** Tests context initialization with invalid provider enum values
- **Coverage:** Tests `dp_init_context` with invalid provider type (999)
- **Test Data:** Uses intentionally invalid provider enum value
- **Expected Behavior:** Should return NULL context
- **Requirements:** 2.1, 2.2

#### 17. `test_list_models_empty_dp.c`
- **Purpose:** Tests model listing when API returns empty model list
- **Coverage:** Tests `dp_list_models` with API that returns zero models
- **Mock Server:** Uses hardcoded localhost:8080 with special API key `EMPTY_LIST`
- **Expected Behavior:** Should succeed but return model list with count=0
- **Dependencies:** Requires curl_global_init/cleanup
- **Requirements:** 2.1, 2.2

#### 18. `test_non_json_error_dp.c`
- **Purpose:** Tests error handling when API returns non-JSON error responses
- **Coverage:** Tests `dp_perform_completion` with API that returns non-JSON error
- **Mock Server:** Uses `DP_MOCK_SERVER` environment variable with special API key `NON_JSON_ERROR`
- **Expected Behavior:** Should fail with HTTP 500 status code
- **Requirements:** 2.1, 2.2

## Migration Considerations

### Mock Server Dependencies
- **17 tests** require `DP_MOCK_SERVER` environment variable
- **1 test** (`test_list_models_empty_dp.c`) uses hardcoded localhost:8080
- Tests should return exit code 77 when mock server is not available

### External Dependencies
- **2 tests** require cJSON library for JSON parsing (Anthropic streaming tests)
- **1 test** requires curl global init/cleanup
- **1 test** uses mkstemp for temporary file creation

### API Key Patterns
Tests use specific API key patterns to trigger different mock server behaviors:
- `AUTH_FAILURE_*` - Triggers HTTP 401 responses
- `RATE_LIMIT_*` - Triggers HTTP 429 responses  
- `STREAM_ERROR_*` - Triggers streaming error scenarios
- `*_UPLOAD` - Triggers file upload scenarios
- `NON_JSON_ERROR` - Triggers non-JSON error responses
- `ABRUPT_STREAM` - Triggers connection termination
- `EMPTY_LIST` - Triggers empty model list response

### Integration Requirements
- All tests must integrate with GNU autotools testsuite
- Tests must return exit code 77 when skipped (mock server unavailable)
- Tests must be adapted to work with current mainline architecture
- Build system must be updated to include new test files

## Test Coverage Gaps Filled

These missing tests provide coverage for:
1. **Authentication error handling** across all providers
2. **Streaming error scenarios** including abrupt termination and mid-stream errors
3. **Token counting edge cases** with multimodal content
4. **File upload validation** including size limits and unsupported providers
5. **Serialization robustness** with malformed input and all content types
6. **Rate limiting handling** for different API endpoints
7. **Input validation** for invalid parameters and edge cases
8. **Non-JSON error response handling**

This comprehensive test coverage will significantly improve the robustness and reliability of the disasterparty library.