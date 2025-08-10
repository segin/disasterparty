# Disaster Party LLM Client Library

Version: 0.5.0

A C library for interacting with OpenAI-compatible, Google Gemini, and Anthropic Claude LLM APIs, with a focus on creating delightful chaos. Supports text and multimodal inputs, file uploads and references, and both regular and streaming responses.

This library is named "Disaster Party" and uses the `dp_` prefix for its C symbols. The library file is `libdisasterparty.so`.

## Architecture

The library is organized into focused modules for maintainability:

- **`disasterparty.h`** - Public API definitions and declarations
- **`dp_private.h`** - Private API and internal structures shared across modules
- **`disasterparty.c`** - Main library entry points and coordination
- **`dp_context.c`** - Context management and initialization
- **`dp_models.c`** - Model listing and provider-specific logic
- **`dp_message.c`** - Message construction and management
- **`dp_request.c`** - HTTP request handling and API calls
- **`dp_stream.c`** - Streaming response processing
- **`dp_serialize.c`** - Conversation serialization/deserialization
- **`dp_file.c`** - File attachment handling
- **`dp_utils.c`** - Utility functions and helpers
- **`dp_constants.c`** - Constants and default values

This modular design improves code maintainability while preserving the complete public API.

## Building

This project uses GNU Autotools.

1.  Run `./autogen.sh` to generate the configure script and Makefiles.
2.  Run `./configure` to check for dependencies and prepare the build.
    * You might need to specify a prefix: `./configure --prefix=/usr/local`
3.  Run `make` to compile the library and tests.
4.  (Optional) Run `make check` to run the test suite. See the "Running Tests" section below for details on required environment variables.
5.  (Optional) Run `sudo make install` to install the library and header files.

## Dependencies

* libcurl (>= 7.20.0)
* libcjson (>= 1.7.10)

## Usage

Include `disasterparty.h` in your C code and link against `-ldisasterparty`.
Ensure your build system also links against libcurl and libcjson (pkg-config can help with this).

Example compilation:
```sh
gcc my_app.c $(pkg-config --cflags --libs disasterparty)
```

See the files in the `tests/` directory for usage examples.

## Testing

For complete testing instructions, environment setup, and test suite information, see **[TESTING.md](TESTING.md)**.