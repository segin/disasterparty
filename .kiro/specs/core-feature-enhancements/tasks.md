# Implementation Plan

- [x] 1. Update version management and build configuration
  - Increment SOVER version in configure.ac to reflect ABI changes
  - Update version-related documentation and build files
  - _Requirements: 1.7, 1.8, 2.8_

- [x] 2. Implement general file attachments support
- [x] 2.1 Extend data structures for file attachments
  - Add DP_CONTENT_PART_FILE_DATA to dp_content_part_type_t enum in disasterparty.h
  - Extend dp_content_part_t struct with file_data field containing mime_type, data, and filename
  - _Requirements: 1.1, 1.7, 1.8_

- [x] 2.2 Implement file attachment helper function
  - Create dp_message_add_file_data_part() function in disasterparty.c
  - Add function declaration to disasterparty.h
  - Implement file reading, base64 encoding, and validation logic
  - _Requirements: 1.1, 1.4, 1.5_

- [x] 2.3 Update payload builders for file attachments
  - Modify build_gemini_json_payload_with_cjson() to handle DP_CONTENT_PART_FILE_DATA
  - Modify build_anthropic_json_payload_with_cjson() to handle DP_CONTENT_PART_FILE_DATA
  - Modify build_openai_json_payload_with_cjson() to handle DP_CONTENT_PART_FILE_DATA
  - _Requirements: 1.2, 1.3_

- [x] 2.4 Update memory cleanup for file attachments
  - Modify dp_free_messages() to properly clean up file_data fields
  - Ensure all file attachment memory is properly freed
  - _Requirements: 1.6_

- [ ] 3. Implement custom user-agent support
- [ ] 3.1 Extend context structure for user-agent
  - Add user_agent field to dp_context_s struct in disasterparty.c
  - _Requirements: 2.8_

- [ ] 3.2 Implement new context initialization function
  - Create dp_init_context_with_app_info() function in disasterparty.c
  - Add function declaration to disasterparty.h
  - Implement user-agent string construction logic
  - _Requirements: 2.1, 2.3, 2.4_

- [ ] 3.3 Update existing initialization function for compatibility
  - Modify dp_init_context() to call dp_init_context_with_app_info() with NULL app info
  - Ensure backward compatibility is maintained
  - _Requirements: 2.2, 2.7_

- [ ] 3.4 Update HTTP requests to use custom user-agent
  - Modify all curl_easy_setopt calls to use context->user_agent instead of DISASTERPARTY_USER_AGENT
  - Update dp_perform_completion, dp_perform_streaming_completion, dp_list_models, and dp_count_tokens functions
  - _Requirements: 2.5_

- [ ] 3.5 Update context cleanup for user-agent
  - Modify dp_destroy_context() to free user_agent field
  - _Requirements: 2.6_

- [ ] 4. Complete token counting implementation
- [ ] 4.1 Complete token counting function implementation
  - Finish the existing dp_count_tokens() implementation in disasterparty.c
  - Ensure proper error handling for OpenAI provider (return error as specified)
  - Complete response parsing for Gemini and Anthropic providers
  - _Requirements: 3.1, 3.2, 3.3, 3.4, 3.5, 3.8_

- [ ] 4.2 Update token counting payload builders
  - Complete build_gemini_count_tokens_json_payload_with_cjson() function
  - Complete build_anthropic_count_tokens_json_payload_with_cjson() function
  - Ensure payloads match provider API specifications
  - _Requirements: 3.2, 3.5_

- [ ] 5. Create comprehensive test suite
- [ ] 5.1 Create file attachments test
  - Write test_file_attachments_dp.c to test various file types and sizes
  - Test error conditions like invalid files and unsupported types
  - Add test to Makefile.am and ensure it builds and runs
  - _Requirements: 1.1, 1.4, 1.5, 1.6_

- [ ] 5.2 Create user-agent test
  - Write test_user_agent_dp.c to test user-agent string construction
  - Test both new and existing initialization functions
  - Verify backward compatibility and memory management
  - Add test to Makefile.am and ensure it builds and runs
  - _Requirements: 2.1, 2.2, 2.3, 2.4, 2.6, 2.7_

- [ ] 5.3 Complete token counting tests
  - Create test_token_counting_dp.c for OpenAI error case testing
  - Enhance test_anthropic_token_counting_dp.c with additional edge cases
  - Create test_gemini_token_counting_dp.c for Gemini provider testing
  - Add all tests to Makefile.am and ensure they build and run
  - _Requirements: 3.1, 3.3, 3.6, 3.7_

- [ ] 6. Update documentation and man pages
- [ ] 6.1 Update API documentation
  - Update DOCUMENTATION.md with new functions and usage examples
  - Document file attachment capabilities and limitations
  - Document user-agent customization options
  - _Requirements: 1.1, 2.1, 3.1_

- [ ] 6.2 Create man pages for new functions
  - Create man page for dp_message_add_file_data_part()
  - Create man page for dp_init_context_with_app_info()
  - Update existing man pages that reference affected structures
  - _Requirements: 1.1, 2.1_

- [ ] 7. Integration and validation
- [ ] 7.1 Build system integration
  - Update Makefile.am to include new test programs
  - Ensure all new code compiles without warnings
  - Verify SOVER increment is properly applied
  - _Requirements: 1.7, 1.8, 2.8_

- [ ] 7.2 Backward compatibility validation
  - Compile and run existing test suite to ensure no regressions
  - Verify that existing code examples still work without modification
  - Test that default behavior is preserved for existing users
  - _Requirements: 2.2, 2.7_