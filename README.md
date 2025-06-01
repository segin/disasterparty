# Disaster Party LLM Client Library

Version: 0.1.1

A C library for interacting with OpenAI-compatible and Google Gemini LLM APIs,
with a focus on creating delightful chaos. Supports text and multimodal inputs,
and both regular and streaming responses.

This library is named "Disaster Party" and uses the `dp_` prefix for its C symbols.
The library file is `libdisasterparty.so`.

## Building

This project uses GNU Autotools.

1.  Run `./autogen.sh` to generate the configure script and Makefiles.
2.  Run `./configure` to check for dependencies and prepare the build.
    * You might need to specify a prefix: `./configure --prefix=/usr/local`
3.  Run `make` to compile the library and tests.
4.  (Optional) Run `make check` to run the test suite.
    * This requires `OPENAI_API_KEY` and `GEMINI_API_KEY` environment variables to be set.
    * For multimodal Gemini tests, `GEMINI_TEST_IMAGE_PATH` or a command-line argument to the test executable is needed.
5.  (Optional) Run `sudo make install` to install the library and header files.

## Dependencies

* libcurl (>= 7.20.0)
* libcjson (>= 1.7.10)

## Usage

Include `disasterparty.h` in your C code and link against `-ldisasterparty`.
Ensure your build system also links against libcurl and libcjson (pkg-config can help with this).

Example compilation:
`gcc my_app.c $(pkg-config --cflags --libs disasterparty)`

See the files in the `tests/` directory for usage examples.

## API Keys

The test programs (and any application using this library) will require API keys
for the respective services (OpenAI, Google Gemini). These are typically provided
via environment variables (`OPENAI_API_KEY`, `GEMINI_API_KEY`).
