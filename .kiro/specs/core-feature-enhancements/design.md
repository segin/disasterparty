# Design Document

## Overview

This design document outlines the implementation of three core feature enhancements for libdisasterparty:

1. **General File Attachments Support** - Adding support for non-image file attachments (PDFs, CSVs, text files, etc.)
2. **Custom User-Agent Support** - Allowing applications to include their name and version in HTTP requests
3. **Token Counting Functionality** - Providing accurate token counting for Gemini and Anthropic providers

The design maintains backward compatibility with existing APIs while following established patterns in the codebase. All new functionality integrates seamlessly with the existing architecture and error handling mechanisms.

## Architecture

### Current State Analysis

Based on code analysis, the current state is:

- **Token Counting**: Already declared in header (`dp_count_tokens`) and partially implemented in source, with working Anthropic test
- **File Attachments**: Documentation exists but no actual implementation in header/source
- **User-Agent**: Currently uses static `DISASTERPARTY_USER_AGENT` constant

### Design Principles

1. **Source Compatibility**: Existing code must compile and function without changes
2. **ABI Evolution**: New functions can be added, existing function signatures cannot change
3. **SOVER Management**: ABI-breaking changes require SOVER increment to maintain library versioning
4. **Provider Abstraction**: Each provider handles features according to their API capabilities
5. **Error Handling**: Consistent error codes and meaningful error messages
6. **Memory Management**: Clear ownership and cleanup patterns

## Components and Interfaces

### 1. General File Attachments Support

#### New Data Structures

```c
// Add to dp_content_part_type_t enum
typedef enum {
    DP_CONTENT_PART_TEXT, 
    DP_CONTENT_PART_IMAGE_URL,
    DP_CONTENT_PART_IMAGE_BASE64,
    DP_CONTENT_PART_FILE_DATA  // New addition
} dp_content_part_type_t;

// Extend dp_content_part_t struct
typedef struct {
    dp_content_part_type_t type;
    char* text;
    char* image_url;
    struct {
        char* mime_type;
        char* data; 
    } image_base64;
    struct {                    // New addition
        char* mime_type;
        char* data;             // Base64-encoded file data
        char* filename;         // Optional filename for context
    } file_data;
} dp_content_part_t;
```

#### New API Functions

```c
// Add file data part to message
bool dp_message_add_file_data_part(dp_message_t* message, 
                                   const char* mime_type, 
                                   const char* base64_data,
                                   const char* filename);
```

#### Provider-Specific Implementation

- **Gemini**: Use Files API for upload-then-reference workflow when file size exceeds inline limits
- **Anthropic**: Use base64 inline data for smaller files, Files API for larger files
- **OpenAI**: Use base64 inline data (limited support, depends on model capabilities)

### 2. Custom User-Agent Support

#### New API Functions

```c
// New initialization function with app info
dp_context_t* dp_init_context_with_app_info(dp_provider_type_t provider, 
                                             const char* api_key,
                                             const char* api_base_url,
                                             const char* app_name,
                                             const char* app_version);
```

#### Internal Changes

```c
// Extend dp_context_s struct
struct dp_context_s {
    dp_provider_type_t provider;
    char* api_key;
    char* api_base_url;
    char* user_agent;  // New field for constructed user-agent string
};
```

#### User-Agent Construction Logic

- If `app_name` provided: `"AppName/AppVersion (disasterparty/DP_VERSION)"`
- If `app_name` is NULL: `"disasterparty/DP_VERSION"`
- Existing `dp_init_context` calls the new function with NULL app info

### 3. Token Counting Functionality

#### Current Implementation Status

The `dp_count_tokens` function is already declared and partially implemented:

```c
int dp_count_tokens(dp_context_t* context,
                    const dp_request_config_t* request_config,
                    size_t* token_count_out);
```

#### Provider-Specific Behavior

- **Gemini**: Call `/v1beta/models/{model}:countTokens` endpoint
- **Anthropic**: Call `/v1/messages/count_tokens` endpoint  
- **OpenAI**: Return error code indicating unsupported operation

#### API Endpoints and Payloads

**Gemini Count Tokens Endpoint:**
- URL: `{base_url}/models/{model}:countTokens`
- Method: POST
- Payload: Similar to completion request but without generation config

**Anthropic Count Tokens Endpoint:**
- URL: `{base_url}/messages/count_tokens`
- Method: POST
- Payload: Similar to completion request but without streaming/generation parameters

## Data Models

### File Attachment Data Flow

1. **Input**: Application provides file path, mime type, and optional filename
2. **Processing**: File is read and base64-encoded
3. **Storage**: File data stored in `dp_content_part_t.file_data` structure
4. **Transmission**: Provider-specific payload builders handle file data appropriately

### User-Agent String Management

1. **Construction**: User-agent string built during context initialization
2. **Storage**: Stored in context structure for reuse across requests
3. **Usage**: Applied to all HTTP requests via curl options
4. **Cleanup**: Freed during context destruction

### Token Count Response Processing

1. **Request**: Send count tokens request to provider endpoint
2. **Response**: Parse JSON response to extract token count
3. **Output**: Return count via output parameter, error code via return value

## Error Handling

### Error Codes and Messages

- **File Attachments**:
  - Invalid file path or unreadable file
  - Unsupported mime type
  - File too large for provider limits
  - Base64 encoding failures

- **User-Agent**:
  - Memory allocation failures during string construction
  - Invalid characters in app name/version

- **Token Counting**:
  - Network errors during API calls
  - Invalid response format from provider
  - Provider-specific error responses
  - Unsupported provider (OpenAI)

### Error Handling Patterns

All functions follow existing patterns:
- Return 0 for success, negative values for errors
- Use output parameters for returning data
- Provide meaningful error messages via stderr
- Clean up allocated resources on error paths

## Testing Strategy

### Unit Tests

1. **File Attachments**:
   - `test_file_attachments_dp.c` - Test various file types and sizes
   - Test provider-specific payload generation
   - Test error conditions (invalid files, unsupported types)

2. **User-Agent**:
   - `test_user_agent_dp.c` - Test user-agent string construction
   - Test backward compatibility with existing init function
   - Test memory management and cleanup

3. **Token Counting**:
   - Complete existing `test_token_counting_dp.c` for OpenAI error case
   - Enhance `test_anthropic_token_counting_dp.c` with edge cases
   - Add `test_gemini_token_counting_dp.c`

### Integration Tests

- Test file attachments with actual API calls
- Verify user-agent strings appear in HTTP requests
- Validate token counts against known test cases

### Backward Compatibility Tests

- Ensure existing code compiles without changes
- Verify existing functionality remains unaffected
- Test that default behavior is preserved

## Implementation Considerations

### Memory Management

- File data is base64-encoded and stored in heap-allocated strings
- User-agent strings are allocated once per context and reused
- All new allocations follow existing cleanup patterns

### Performance

- File encoding happens once during message construction
- User-agent string constructed once during context initialization
- Token counting adds network round-trip but provides valuable information

### Security

- File paths are validated before reading
- Base64 encoding prevents binary data issues
- User-agent strings are sanitized to prevent injection

### Provider Compatibility

- Each provider handles file attachments according to their capabilities
- Token counting gracefully fails for unsupported providers
- User-agent strings are compatible with all HTTP-based APIs

### Version Management

- **SOVER Increment Required**: The changes to `dp_content_part_type_t` enum and `dp_content_part_t` struct, as well as the extension of `dp_context_s` struct, constitute ABI-breaking changes
- **Version Coordination**: SOVER increment must be coordinated with the build system (configure.ac, Makefile.am)
- **Documentation Updates**: Version changes must be reflected in documentation and changelog