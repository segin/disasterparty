# Architecture Overview
This document serves as a critical, living template designed to equip agents with a rapid and comprehensive understanding of the codebase's architecture, enabling efficient navigation and effective contribution from day one. Update this document as the codebase evolves.

## 1. Project Structure
This section provides a high-level overview of the project's directory and file structure, categorised by architectural layer or major functional area.

[Project Root]/
├── src/                  # Main source code for the library
│   ├── disasterparty.c   # Core logic, payload construction, and response parsing
│   ├── disasterparty.h   # Public API header
│   ├── dp_context.c      # Context management (API keys, config)
│   ├── dp_request.c      # Network request handling (libcurl wrapper)
│   ├── dp_message.c      # Message and content part manipulation helpers
│   ├── dp_stream.c       # Streaming response processing logic
│   ├── dp_serialize.c    # Conversation serialization/deserialization
│   ├── dp_models.c       # Model listing functionality
│   ├── dp_file.c         # File upload and handling
│   └── dp_private.h      # Internal private header
├── tests/                # Unit, integration, and fuzz tests
│   ├── mock-server/      # Mock server for testing without live APIs
│   └── ...               # Individual test C files (e.g., test_openai_text_dp.c)
├── man/                  # Man pages source files (.3dp, .7dp)
├── docs/                 # Additional documentation (ROADMAP.md, etc.)
├── build-aux/            # Autotools auxiliary files
├── m4/                   # Autotools macros
├── configure.ac          # Autoconf configuration script
└── Makefile.am           # Automake configuration file

## 2. High-Level System Diagram

[Application] <--(C API)--> [libdisasterparty]
                                   |
                                   | (JSON Payload Construction)
                                   v
                                [libcurl]
                                   |
                                   | (HTTPS)
                                   v
        +--------------------------+--------------------------+
        |                          |                          |
   [OpenAI API]            [Gemini API]              [Anthropic API]

## 3. Core Components

### 3.1. Library Core (libdisasterparty)

Name: libdisasterparty

Description: A lightweight, unopinionated C library for interacting with Large Language Model (LLM) APIs. It abstracts the differences between providers (OpenAI, Gemini, Anthropic) into a unified interface.

Technologies: C11, libcurl, cJSON

Key Modules:
*   **Context Manager (`dp_context`):** Holds state (API keys, base URLs, provider type).
*   **Message Builder (`dp_message`):** manages the list of messages and multimodal content parts (text, images, files, tool calls).
*   **Request Engine (`dp_request`):** Orchestrates the HTTP request lifecycle.
*   **Payload Builder (`disasterparty.c`):** Converts internal structs into provider-specific JSON schemas.
*   **Response Parser (`disasterparty.c`, `dp_stream.c`):** Parses JSON responses and handles Server-Sent Events (SSE) for streaming.

## 4. Data Stores

This project is a client library and does not maintain its own persistent data store. It allows applications to serialize conversation history to files using `dp_serialize`.

## 5. External Integrations / APIs

### 5.1. OpenAI Compatible API
Purpose: LLM Chat Completions.
Integration Method: REST API (`/v1/chat/completions`), SSE for streaming.

### 5.2. Google Gemini API
Purpose: Multimodal LLM generation.
Integration Method: REST API (`/v1beta/models/...:generateContent`), SSE for streaming (`streamGenerateContent`).

### 5.3. Anthropic API
Purpose: Claude LLM interaction.
Integration Method: REST API (`/v1/messages`), custom SSE event format for streaming.

## 6. Deployment & Infrastructure

Type: Shared Library (`.so`, `.dylib`, `.dll`)
Build System: GNU Autotools (`autoconf`, `automake`, `libtool`).
CI: GitHub Actions (implied by typical setup).

## 7. Security Considerations

*   **API Keys:** Handled via `dp_context_t`. The library expects the host application to manage the secure storage and retrieval of keys. Keys are resident in memory during the context's lifecycle.
*   **Transport Security:** Relies on `libcurl` for TLS/SSL encryption to ensure data privacy in transit.
*   **Memory Management:** The library performs manual memory management. Users must ensure they call corresponding free functions (`dp_free_response_content`, `dp_free_messages`, `dp_destroy_context`) to prevent leaks.

## 8. Development & Testing Environment

Local Setup:
1.  `./autogen.sh`
2.  `./configure`
3.  `make`
4.  `make check`

Testing Frameworks: Custom C test harness driven by `automake`'s `check` target. Tests range from simple unit tests to network-bound integration tests (which require API keys in env vars).
Fuzzing: LLVM libFuzzer support for response parsing (`tests/test_fuzz_response_parser_dp.c`).

## 9. Future Considerations / Roadmap

See `docs/ROADMAP.md` for detailed planning.
*   **Tool Calling:** (Implemented in 0.6.0)
*   **Thinking Tokens:** (Implemented in 0.6.0)
*   **Advanced Multimodality:** Ongoing support for new file types and modalities.
*   **Real-time Audio:** Potential future expansion.

## 10. Project Identification

Project Name: Disaster Party
Repository URL: https://github.com/segin/disasterparty
Primary Contact/Team: segin2005@gmail.com
Date of Last Update: 2024-05-23

## 11. Glossary / Acronyms

*   **LLM:** Large Language Model
*   **SSE:** Server-Sent Events (used for streaming responses)
*   **ABI:** Application Binary Interface
*   **Multimodal:** Capable of processing mixed inputs (text, images, files).