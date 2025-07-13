# Testing Disaster Party

This document outlines how to use the mock server for testing the Disaster Party library.

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

```