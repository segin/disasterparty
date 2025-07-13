# Testing Disaster Party

This document outlines how to test the Disaster Party LLM client library, including running the main test suite and utilizing the mock server for specific scenarios.

## Running the Test Suite

The test suite can be run with `make check`. The tests make live API calls and require certain environment variables to be set. Tests for a specific provider or feature will be skipped if their required variables are not found.

To run the full test suite, the following environment variables are needed:

### API Keys
*   **`OPENAI_API_KEY`**: Your API key for OpenAI or any OpenAI-compatible service.
*   **`GEMINI_API_KEY`**: Your API key for Google's Gemini API.
*   **`ANTHROPIC_API_KEY`**: Your API key for Anthropic's Claude API.

### Image Paths (for Multimodal Tests)
*   **`GEMINI_TEST_IMAGE_PATH`**: A valid path to a JPEG or PNG image for Gemini multimodal tests.
*   **`ANTHROPIC_TEST_IMAGE_PATH`**: A valid path to a JPEG or PNG image for Anthropic multimodal tests.
*   **`TEST_IMAGE_PATH_1`**: Path to the first image for the generic inline context test.
*   **`TEST_IMAGE_PATH_2`**: Path to the second image for the generic inline context test.

### Optional Variables
*   **`OPENAI_API_BASE_URL`**: To test against a custom OpenAI-compatible endpoint instead of the default `https://api.openai.com/v1`.

### Example
```sh
export OPENAI_API_KEY="sk-..."
export GEMINI_API_KEY="..."
export ANTHROPIC_API_KEY="..."
export GEMINI_TEST_IMAGE_PATH="tests/assets/sample.png"
export ANTHROPIC_TEST_IMAGE_PATH="tests/assets/sample.png"
export TEST_IMAGE_PATH_1="tests/assets/sample.png"
export TEST_IMAGE_PATH_2="tests/assets/sample.pdf"

make check
```

### Full List of Unit Tests
The test suite currently includes the following test programs:

*   `test_openai_text_dp`
*   `test_openai_multimodal_dp`
*   `test_openai_streaming_dp`
*   `test_openai_list_models_dp`
*   `test_openai_streaming_multimodal_dp`
*   `test_gemini_text_dp`
*   `test_gemini_multimodal_dp`
*   `test_gemini_streaming_dp`
*   `test_gemini_list_models_dp`
*   `test_gemini_streaming_multimodal_dp`
*   `test_gemini_file_completion_dp`
*   `test_gemini_file_attachment_dp`
*   `test_anthropic_text_dp`
*   `test_anthropic_multimodal_dp`
*   `test_anthropic_streaming_dp`
*   `test_anthropic_streaming_detailed_dp`
*   `test_anthropic_list_models_dp`
*   `test_anthropic_streaming_multimodal_dp`
*   `test_anthropic_all_event_types_dp`
*   `test_serialization_dp`
*   `test_inline_multimodal_dp`
*   `test_error_handling_dp`
*   `test_parameters_dp`
*   `test_anthropic_streaming_multimodal_detailed_dp`
*   `test_multiprovider_multithread_dp`
*   `test_abrupt_stream_dp`
*   `test_non_json_error_dp`
*   `test_list_models_empty_dp`
*   `test_anthropic_token_counting_dp`
*   `test_gemini_token_counting_dp`
*   `test_unsupported_file_uploads_dp`
*   `test_user_agent_dp`
*   `test_invalid_provider_dp`

## Mock Server

A Python-based mock server is provided in `tests/mock-server/main.py` to simulate various API responses for different providers (OpenAI, Google Gemini, Anthropic). This allows for testing specific error conditions and behaviors without making actual network requests to external APIs.

### Running the Mock Server

To run the mock server, navigate to the `tests/mock-server/` directory and execute the `main.py` script:

```bash
cd tests/mock-server/
python main.py &
```

The server will run on `http://localhost:8080`. Ensure this port is available.

### Scenario Signaling

The mock server uses the **API key** provided to the `dp_init_context` function to determine which test scenario to emulate. This is done by inspecting the `Authorization` header (for OpenAI-compatible), `x-api-key` header (for Anthropic), or `key` query parameter (for Google Gemini) in incoming requests.

For example, if a test needs to trigger a "non-JSON error" scenario, the `dp_init_context` call in that test would use `"NON_JSON_ERROR"` as its API key.

```c
dp_context_t* context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, "NON_JSON_ERROR", mock_server_url);
```

The mock server's `main.py` then checks for this specific API key to return the corresponding mock response.

### Test Skipping

Tests that rely on the mock server will automatically be skipped if the `DP_MOCK_SERVER` environment variable is not set. This variable should point to the base URL of the running mock server (e.g., `http://localhost:8080/v1`).

For example, in a test file:

```c
const char* mock_server_url = getenv("DP_MOCK_SERVER");
if (!mock_server_url) {
    printf("SKIP: DP_MOCK_SERVER environment variable not set.\n");
    return 77; // Standard exit code for skipped tests in Autotools
}
```

