# Implementation Plan

- [x] 1. Set up infrastructure and checkout test-expansions branch
  - Create a subdirectory checkout of the test-expansions branch for analysis
  - Examine the modular structure and identify the exact file organization pattern
  - _Requirements: 3.1, 3.2_

- [x] 2. Create private header and shared infrastructure
  - Create `src/dp_private.h` with internal structure definitions and private function declarations
  - Move internal structures from `disasterparty.c` to the private header
  - Define shared internal types and constants needed across modules
  - _Requirements: 1.1, 1.3_

- [x] 3. Extract constants and version management
  - Create `src/dp_constants.c` with default URLs, version info, and provider constants
  - Move `dp_get_version()` and all constant definitions from main file
  - Update build system to include the new source file
  - _Requirements: 1.1, 1.2, 1.5_

- [x] 4. Extract utility functions module
  - Create `src/dp_utils.c` with common utility functions and string manipulation
  - Move `dp_internal_strdup()`, `dp_safe_asprintf()`, and token counting functions
  - Include memory management helpers and other utility functions
  - _Requirements: 1.1, 1.2, 1.5_

- [x] 5. Extract context management module
  - Create `src/dp_context.c` with context initialization, cleanup, and management functions
  - Move `dp_init_context()` family of functions and context validation
  - Include API key and base URL management functions
  - _Requirements: 1.1, 1.2, 1.5_

- [x] 6. Extract HTTP request handling module
  - Create `src/dp_request.c` with HTTP request construction and cURL integration
  - Move `write_memory_callback()` and core HTTP request processing functions
  - Include response processing and error handling for HTTP operations
  - _Requirements: 1.1, 1.2, 1.5_

- [x] 7. Extract message management module
  - Create `src/dp_message.c` with message creation, manipulation, and validation
  - Move all `dp_message_*()` family of functions and content part handling
  - Include message cleanup and validation functions
  - _Requirements: 1.1, 1.2, 1.5_

- [-] 8. Extract streaming functionality module
  - Create `src/dp_stream.c` with streaming response processing and callback management
  - Move streaming callback processors and provider-specific stream handlers
  - Include stream state management and parsing logic
  - _Requirements: 1.1, 1.2, 1.5_

- [ ] 9. Extract serialization module
  - Create `src/dp_serialize.c` with conversation serialization and JSON handling
  - Move `dp_serialize_conversation()`, `dp_deserialize_conversation()` and related functions
  - Include JSON parsing utilities and format validation
  - _Requirements: 1.1, 1.2, 1.5_

- [ ] 10. Extract file handling module
  - Create `src/dp_file.c` with file attachment processing and validation
  - Move `dp_message_add_file_data_part()` and file processing utilities
  - Include MIME type detection and file encoding functions
  - _Requirements: 1.1, 1.2, 1.5_

- [ ] 11. Extract model management module
  - Create `src/dp_models.c` with model listing and provider-specific logic
  - Move `dp_list_models()` and provider-specific model functions
  - Include model validation and capabilities handling
  - _Requirements: 1.1, 1.2, 1.5_

- [ ] 12. Update build system configuration
  - Modify `src/Makefile.am` to include all new source files in the build
  - Ensure proper dependency tracking and linking for all modules
  - Verify that `-Wall -Werror` compilation works with the new structure
  - _Requirements: 1.5, 5.1_

- [ ] 13. Validate modular build and API compatibility
  - Compile the restructured codebase and verify no warnings or errors
  - Run existing tests to ensure public API compatibility is maintained
  - Verify that all existing functionality works with the new modular structure
  - _Requirements: 1.3, 1.4, 1.5_

- [ ] 14. Identify and catalog missing tests from test-expansions
  - Compare test directories between main and test-expansions branches
  - Create a list of test files that exist in test-expansions but not in main
  - Document the purpose and coverage of each missing test
  - _Requirements: 2.1, 2.2_

- [ ] 15. Migrate authentication failure tests
  - Adapt `test_anthropic_auth_failure_dp.c`, `test_gemini_auth_failure_dp.c`, and `test_openai_auth_failure_dp.c`
  - Modify tests to work with current mainline architecture and API
  - Integrate tests with GNU autotools testsuite and ensure proper exit codes
  - _Requirements: 2.2, 2.3, 2.4_

- [ ] 16. Migrate streaming and error handling tests
  - Adapt `test_abrupt_stream_dp.c`, `test_anthropic_streaming_error_dp.c`, and `test_anthropic_streaming_ping_dp.c`
  - Modify tests to work with current streaming implementation
  - Ensure tests integrate properly with testsuite and handle skip conditions
  - _Requirements: 2.2, 2.3, 2.4_

- [ ] 17. Migrate serialization and file handling tests
  - Adapt `test_deserialize_malformed_file_dp.c`, `test_serialization_all_parts_dp.c`
  - Migrate file upload tests: `test_unsupported_file_uploads_dp.c`, `test_upload_large_file_dp.c`, `test_upload_zero_byte_file_dp.c`
  - Ensure tests work with current serialization and file handling implementation
  - _Requirements: 2.2, 2.3, 2.4_

- [ ] 18. Migrate token counting and rate limiting tests
  - Adapt `test_anthropic_multimodal_token_count_dp.c`, `test_gemini_multimodal_token_count_dp.c`
  - Migrate `test_rate_limit_completion_dp.c` and `test_rate_limit_list_models_dp.c`
  - Ensure tests work with current token counting and rate limiting logic
  - _Requirements: 2.2, 2.3, 2.4_

- [ ] 19. Migrate remaining edge case and error tests
  - Adapt `test_invalid_provider_dp.c`, `test_list_models_empty_dp.c`, `test_non_json_error_dp.c`
  - Ensure all edge case tests work with current error handling implementation
  - Verify proper integration with testsuite and exit code handling
  - _Requirements: 2.2, 2.3, 2.4_

- [ ] 20. Update testsuite configuration
  - Add all migrated tests to the GNU autotools testsuite configuration
  - Verify that all tests run properly and report results correctly
  - Ensure tests return exit code 77 when skipped as required
  - _Requirements: 2.3, 2.4_

- [ ] 21. Run comprehensive test validation
  - Execute the complete test suite to verify all tests pass
  - Validate that both existing and migrated tests work correctly
  - Ensure no regressions were introduced during the restructuring
  - _Requirements: 2.2, 2.3, 2.4_

- [ ] 22. Clean up test-expansions checkout and branch
  - Remove the test-expansions subdirectory checkout used for analysis
  - Delete the test-expansions branch from the local repository
  - Push the branch deletion to origin to clean up the remote repository
  - _Requirements: 4.1, 4.2, 4.3_

- [ ] 23. Update documentation and commit changes
  - Update README.md to reflect the new modular file organization
  - Document the new file structure and any changes to build process
  - Commit all changes with descriptive messages and push to repository
  - _Requirements: 4.4, 5.1, 5.2_

- [ ] 24. Final validation and cleanup
  - Perform final build and test validation to ensure everything works
  - Verify that no traces of the abandoned branch remain in working directory
  - Confirm that the refactoring is complete and all requirements are met
  - _Requirements: 4.4, 5.3, 5.4_