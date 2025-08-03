# Requirements Document

## Introduction

This document outlines the requirements for implementing Section 2 of the libdisasterparty roadmap: "Medium-Term Goals / Core Feature Enhancements". These features will enhance the core capabilities of the library by adding support for general file attachments, custom user-agent strings, and token counting functionality. The implementation will build upon the existing architecture while maintaining backward compatibility and following established patterns in the codebase.

## Requirements

### Requirement 1: General File Attachments Support

**User Story:** As a developer using libdisasterparty, I want to attach non-image files (PDFs, CSVs, text files, etc.) to my messages, so that I can leverage AI models' document processing capabilities.

#### Acceptance Criteria

1. WHEN a developer calls a new file attachment function THEN the system SHALL support base64-encoded file data for PDFs, CSVs, and other document types
2. WHEN the system processes file attachments THEN it SHALL use the appropriate two-step upload-then-reference workflow for Gemini and Anthropic APIs
3. WHEN building payloads THEN the system SHALL correctly format file data according to each provider's JSON structure requirements
4. WHEN a file attachment is added THEN the system SHALL validate the file type and encoding before processing
5. IF an unsupported file type is provided THEN the system SHALL return an appropriate error code
6. WHEN multiple file attachments are used THEN the system SHALL handle them correctly alongside existing text and image parts
7. WHEN the dp_content_part_type_t enum is extended THEN the system SHALL increment the SOVER version to reflect the ABI change
8. WHEN the dp_content_part_t struct is extended THEN the system SHALL increment the SOVER version to reflect the ABI change

### Requirement 2: Custom User-Agent Support

**User Story:** As an application developer integrating libdisasterparty, I want to include my application name and version in HTTP requests, so that API providers can track usage and I can identify my application's requests in logs.

#### Acceptance Criteria

1. WHEN a new initialization function is created THEN the system SHALL provide dp_init_context_with_app_info() that accepts app_name and app_version parameters
2. WHEN the existing dp_init_context function is called THEN it SHALL continue to work without any changes to maintain source compatibility
3. WHEN app_name and app_version are provided THEN the system SHALL construct a user-agent string in the format "AppName/AppVersion (disasterparty/DP_VERSION)"
4. WHEN app_name is NULL THEN the system SHALL default to "disasterparty/DP_VERSION" format
5. WHEN making HTTP requests THEN the system SHALL use the constructed user-agent string for all curl operations
6. WHEN the context is destroyed THEN the system SHALL properly clean up any allocated user-agent string memory
7. WHEN existing source code is recompiled THEN it SHALL build and function identically to before without any modifications
8. WHEN the dp_context_s struct is extended with user_agent field THEN the system SHALL increment the SOVER version to reflect the ABI change

### Requirement 3: Token Counting Functionality

**User Story:** As a developer managing AI costs and context windows, I want to count tokens in my prompts before sending them, so that I can optimize my requests and avoid exceeding limits.

#### Acceptance Criteria

1. WHEN a developer calls the token counting function THEN the system SHALL return an accurate token count for the given prompt
2. WHEN using Gemini or Anthropic providers THEN the system SHALL call their respective countTokens API endpoints
3. WHEN using the OpenAI API target THEN the system SHALL return an appropriate error indicating token counting is not supported for this provider type
4. WHEN the token counting request fails THEN the system SHALL return proper error codes and messages
5. WHEN the API response is received THEN the system SHALL parse the token count correctly and return it via output parameter
6. WHEN invalid parameters are provided THEN the system SHALL validate inputs and return appropriate error codes
7. WHEN network errors occur THEN the system SHALL handle them gracefully and return meaningful error information
8. WHEN the OpenAI API target is retargeted to other providers THEN the system SHALL still return the not-supported error rather than attempting client-side tokenization