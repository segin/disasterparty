# Disaster Party LLM Client Library

Version: 0.3.0

A C library for interacting with OpenAI-compatible, Google Gemini, and Anthropic Claude LLM APIs, with a focus on creating delightful chaos. Supports text and multimodal inputs, and both regular and streaming responses.

This library is named "Disaster Party" and uses the `dp_` prefix for its C symbols. The library file is `libdisasterparty.so`.

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

The test suite can be run with `make check`. The tests make live API calls and require certain environment variables to be set. Tests for a specific provider or feature will be skipped if their required variables are not found.

To run the full test suite, the following environment variables are needed:

### API Keys
* **`OPENAI_API_KEY`**: Your API key for OpenAI or any OpenAI-compatible service.
* **`GEMINI_API_KEY`**: Your API key for Google's Gemini API.
* **`ANTHROPIC_API_KEY`**: Your API key for Anthropic's Claude API.

### Image Paths (for Multimodal Tests)
* **`GEMINI_TEST_IMAGE_PATH`**: A valid path to a JPEG or PNG image for Gemini multimodal tests.
* **`ANTHROPIC_TEST_IMAGE_PATH`**: A valid path to a JPEG or PNG image for Anthropic multimodal tests.
* **`TEST_IMAGE_PATH_1`**: Path to the first image for the generic inline context test.
* **`TEST_IMAGE_PATH_2`**: Path to the second image for the generic inline context test.

### Optional Variables
* **`OPENAI_API_BASE_URL`**: To test against a custom OpenAI-compatible endpoint instead of the default `https://api.openai.com/v1`.

### Example
```sh
export OPENAI_API_KEY="sk-..."
export GEMINI_API_KEY="..."
export ANTHROPIC_API_KEY="..."
export GEMINI_TEST_IMAGE_PATH="/path/to/your/image.jpg"
export ANTHROPIC_TEST_IMAGE_PATH="/path/to/your/image.jpg"
export TEST_IMAGE_PATH_1="/path/to/your/first_image.jpg"
export TEST_IMAGE_PATH_2="/path/to/your/second_image.jpg"

make check
```

### Full List of Unit Tests
The test suite currently includes the following 21 test programs:

* `test_openai_text_dp`
* `test_openai_multimodal_dp`
* `test_openai_streaming_dp`
* `test_openai_list_models_dp`
* `test_openai_streaming_multimodal_dp`
* `test_gemini_text_dp`
* `test_gemini_multimodal_dp`
* `test_gemini_streaming_dp`
* `test_gemini_list_models_dp`
* `test_gemini_streaming_multimodal_dp`
* `test_anthropic_text_dp`
* `test_anthropic_multimodal_dp`
* `test_anthropic_streaming_dp`
* `test_anthropic_streaming_detailed_dp`
* `test_anthropic_list_models_dp`
* `test_anthropic_streaming_multimodal_dp`
* `test_serialization_dp`
* `test_inline_multimodal_dp`
* `test_error_handling_dp`
* `test_parameters_dp`
* `test_anthropic_streaming_multimodal_detailed_dp`
