# Design Document

## Overview

This design outlines the comprehensive refactoring of the disasterparty codebase to improve maintainability by splitting the monolithic 2203-line `disasterparty.c` file into multiple focused modules. The design is based on the organizational structure discovered in the `test-expansions` branch, which provides a proven blueprint for logical code separation.

The refactoring will maintain complete API compatibility while improving code organization, making the codebase more manageable for both human developers and AI models that struggle with large single files.

## Architecture

### Current State Analysis
- **Main branch**: Single monolithic `src/disasterparty.c` (2203 lines) with `src/disasterparty.h`
- **Test-expansions branch**: Well-organized modular structure with 10 focused C files plus headers
- **Missing tests**: Several test files exist in test-expansions but not in main branch

### Target Architecture
The new modular architecture will follow the test-expansions pattern with these core modules:

```
src/
├── disasterparty.h          # Public API (unchanged)
├── dp_private.h             # Private API and internal structures
├── dp_context.c             # Context management and initialization
├── dp_models.c              # Model listing and provider-specific logic
├── dp_message.c             # Message construction and management
├── dp_request.c             # HTTP request handling and API calls
├── dp_stream.c              # Streaming response processing
├── dp_serialize.c           # Conversation serialization/deserialization
├── dp_file.c                # File attachment handling
├── dp_utils.c               # Utility functions and helpers
└── dp_constants.c           # Constants and default values
```

## Components and Interfaces

### 1. Context Management (`dp_context.c`)
**Responsibilities:**
- Context initialization and cleanup
- API key and base URL management
- Provider configuration
- User agent handling

**Key Functions:**
- `dp_init_context()` and variants
- `dp_free_context()`
- Context validation functions

### 2. Models (`dp_models.c`)
**Responsibilities:**
- Model listing for all providers
- Provider-specific model handling
- Model validation and capabilities

**Key Functions:**
- `dp_list_models()`
- Provider-specific model functions

### 3. Message Management (`dp_message.c`)
**Responsibilities:**
- Message creation and manipulation
- Content part management
- Message validation

**Key Functions:**
- `dp_message_*()` family of functions
- Content part handling
- Message cleanup

### 4. Request Handling (`dp_request.c`)
**Responsibilities:**
- HTTP request construction
- cURL integration
- Response processing
- Error handling

**Key Functions:**
- `dp_completion()`
- HTTP utility functions
- Response parsing

### 5. Streaming (`dp_stream.c`)
**Responsibilities:**
- Streaming response processing
- Callback management
- Stream parsing for different providers

**Key Functions:**
- Streaming callback processors
- Provider-specific stream handlers
- Stream state management

### 6. Serialization (`dp_serialize.c`)
**Responsibilities:**
- Conversation serialization to JSON
- Deserialization from JSON
- Format validation

**Key Functions:**
- `dp_serialize_conversation()`
- `dp_deserialize_conversation()`
- JSON handling utilities

### 7. File Handling (`dp_file.c`)
**Responsibilities:**
- File attachment processing
- MIME type detection
- File validation and encoding

**Key Functions:**
- `dp_message_add_file_data_part()`
- File processing utilities
- Attachment validation

### 8. Utilities (`dp_utils.c`)
**Responsibilities:**
- Common utility functions
- String manipulation
- Memory management helpers
- Token counting utilities

**Key Functions:**
- `dp_count_tokens()`
- String utilities
- Memory helpers

### 9. Constants (`dp_constants.c`)
**Responsibilities:**
- Default URLs and configuration
- Version information
- Provider-specific constants

**Key Functions:**
- `dp_get_version()`
- Constant definitions

### 10. Private Headers (`dp_private.h`)
**Responsibilities:**
- Internal structure definitions
- Private function declarations
- Shared internal types

## Data Models

### Internal Structures
The private header will contain:
- `struct dp_context_s` - Context implementation
- `memory_struct_t` - HTTP response handling
- `stream_processor_t` - Streaming state management
- `anthropic_stream_processor_t` - Anthropic-specific streaming

### Public API Preservation
All public structures and functions in `disasterparty.h` remain unchanged:
- `dp_context_t` (opaque pointer)
- `dp_message_t` and related types
- All public function signatures
- All public constants and enums

## Error Handling

### Consistency Across Modules
- Maintain existing error handling patterns
- Use consistent return codes across modules
- Preserve existing error reporting mechanisms
- Ensure proper cleanup in all error paths

### Module-Specific Error Handling
- Each module handles its own internal errors
- Errors propagate up through the call stack
- Memory cleanup handled at appropriate levels
- HTTP errors handled in request module

## Testing Strategy

### Test Migration and Integration
1. **Identify Missing Tests**: Compare test-expansions and main branch test suites
2. **Adapt Tests**: Modify test-expansions tests to work with main branch architecture
3. **Integration**: Ensure all tests integrate with GNU autotools testsuite
4. **Validation**: All tests must return exit code 77 when skipped

### Missing Tests from test-expansions Branch
Based on directory comparison, these tests need to be migrated:
- `test_abrupt_stream_dp.c`
- `test_anthropic_auth_failure_dp.c`
- `test_anthropic_multimodal_token_count_dp.c`
- `test_anthropic_streaming_error_dp.c`
- `test_anthropic_streaming_ping_dp.c`
- `test_deserialize_malformed_file_dp.c`
- `test_gemini_auth_failure_dp.c`
- `test_gemini_multimodal_token_count_dp.c`
- `test_invalid_provider_dp.c`
- `test_list_models_empty_dp.c`
- `test_non_json_error_dp.c`
- `test_openai_auth_failure_dp.c`
- `test_rate_limit_completion_dp.c`
- `test_rate_limit_list_models_dp.c`
- `test_serialization_all_parts_dp.c`
- `test_unsupported_file_uploads_dp.c`
- `test_upload_large_file_dp.c`
- `test_upload_zero_byte_file_dp.c`

### Build System Updates
- Update `src/Makefile.am` to include all new source files
- Ensure proper dependency tracking
- Maintain `-Wall -Werror` compliance across all modules
- Update any necessary build configuration

## Implementation Phases

### Phase 1: Infrastructure Setup
- Create modular file structure
- Set up private header with shared definitions
- Update build system configuration

### Phase 2: Core Module Extraction
- Extract context management
- Extract utility functions
- Extract constants and version info

### Phase 3: Feature Module Extraction
- Extract message handling
- Extract request processing
- Extract streaming functionality

### Phase 4: Specialized Module Extraction
- Extract serialization functionality
- Extract file handling
- Extract model management

### Phase 5: Test Integration
- Migrate missing tests from test-expansions
- Adapt tests to main branch architecture
- Validate all tests pass

### Phase 6: Cleanup and Validation
- Remove test-expansions checkout
- Delete test-expansions branch
- Final validation and documentation updates

## Migration Strategy

### Code Movement Approach
1. **Preserve Git History**: Use careful extraction to maintain code provenance
2. **Incremental Validation**: Test after each module extraction
3. **API Compatibility**: Verify public API remains unchanged throughout
4. **Build Validation**: Ensure clean compilation with `-Wall -Werror` at each step

### Dependency Management
- Minimize inter-module dependencies
- Use private header for shared internal definitions
- Maintain clear module boundaries
- Avoid circular dependencies

## Risk Mitigation

### Compatibility Risks
- **Mitigation**: Extensive testing after each module extraction
- **Validation**: API compatibility tests throughout process
- **Rollback**: Git-based rollback strategy for any issues

### Build System Risks
- **Mitigation**: Incremental build system updates
- **Validation**: Test builds after each change
- **Documentation**: Clear build system documentation

### Test Integration Risks
- **Mitigation**: Careful adaptation of test-expansions tests
- **Validation**: Ensure all tests integrate properly with autotools
- **Fallback**: Skip problematic tests with exit code 77 if needed