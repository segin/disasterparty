# Testing Environment Setup

This document describes how to set up the testing environment for the Disaster Party library, including required environment variables and mock server configuration.

## Environment Variables

The test suite requires various environment variables to run different types of tests. Tests will skip with exit code 77 if required variables are not set.

### API Keys

These are required for tests that make actual API calls to the respective services:

```bash
export OPENAI_API_KEY="your-openai-api-key"
export GEMINI_API_KEY="your-gemini-api-key" 
export ANTHROPIC_API_KEY="your-anthropic-api-key"
```

### API Base URLs

Optional base URLs for API endpoints (useful for local testing or alternative endpoints):

```bash
export OPENAI_API_BASE_URL="https://api.openai.com/v1"  # Optional, defaults to OpenAI
```

### Mock Server

Required for tests that simulate error conditions, rate limiting, and other edge cases:

```bash
export DP_MOCK_SERVER="http://localhost:8080"
```

### Test Image Paths

Required for multimodal tests that need image files:

```bash
export GEMINI_TEST_IMAGE_PATH="tests/assets/test_image_small.jpg"
export ANTHROPIC_TEST_IMAGE_PATH="tests/assets/test_image_small.jpg"
export TEST_IMAGE_PATH_1="tests/assets/test_image_small.jpg"
export TEST_IMAGE_PATH_2="tests/assets/test_image_medium.png"
```

### Model Names (Optional)

These allow overriding default models used in tests:

```bash
export OPENAI_MODEL="gpt-4o-mini"                    # Default: gpt-4.1-nano
export OPENAI_MODEL_VISION="gpt-4o"                 # Default: gpt-4o
export GEMINI_MODEL="gemini-2.0-flash"              # Default: gemini-2.0-flash
export GEMINI_MODEL_VISION="gemini-2.0-flash"      # Default: gemini-2.0-flash
export ANTHROPIC_MODEL="claude-3-haiku-20240307"   # Default: claude-3-haiku-20240307
```

## Mock Server Setup

The mock server is required for tests that simulate various error conditions and edge cases.

### Prerequisites

Ensure Python 3 and Flask are installed:

```bash
python3 --version  # Should be 3.6+
pip3 install flask
```

### Starting the Mock Server

#### Option 1: Using the start script

```bash
cd tests/mock-server
./start_server.sh [port]
```

The default port is 8080. The script will display the URL to set for `DP_MOCK_SERVER`.

#### Option 2: Direct Python execution

```bash
cd tests/mock-server
python3 main.py --port 8080
```

#### Option 3: Background execution

```bash
cd tests/mock-server
python3 main.py --port 8080 &
export DP_MOCK_SERVER=http://localhost:8080
```

### Mock Server Test Scenarios

The mock server responds to different scenarios based on the API key used:

- `AUTH_FAILURE_OPENAI` - Returns HTTP 401 for OpenAI endpoints
- `AUTH_FAILURE_GEMINI` - Returns HTTP 401 for Gemini endpoints  
- `AUTH_FAILURE_ANTHROPIC` - Returns HTTP 401 for Anthropic endpoints
- `ABRUPT_STREAM` - Simulates abruptly terminated streaming connections
- `STREAM_ERROR_ANTHROPIC` - Simulates mid-stream error events
- `STREAM_PING_ANTHROPIC` - Simulates ping events in streams
- `RATE_LIMIT_COMPLETION` - Returns HTTP 429 for completion requests
- `RATE_LIMIT_LIST_MODELS` - Returns HTTP 429 for model listing
- `ZERO_BYTE_FILE` - Tests handling of empty files
- `LARGE_FILE_UPLOAD` - Tests handling of oversized files
- `NON_JSON_ERROR` - Returns non-JSON error responses
- `EMPTY_LIST` - Returns empty model lists

## Complete Setup Script

Here's a complete script to set up the testing environment:

```bash
#!/bin/bash

# Set API keys (replace with your actual keys)
export OPENAI_API_KEY="your-openai-api-key"
export GEMINI_API_KEY="your-gemini-api-key"
export ANTHROPIC_API_KEY="your-anthropic-api-key"

# Optional: Set custom base URL for OpenAI (for local testing)
# export OPENAI_API_BASE_URL="http://localhost:11434/v1"

# Start mock server in background
cd tests/mock-server
python3 main.py --port 8080 &
MOCK_PID=$!
cd ../..

# Set mock server URL
export DP_MOCK_SERVER="http://localhost:8080"

# Set test image paths using provided assets
export GEMINI_TEST_IMAGE_PATH="tests/assets/test_image_small.jpg"
export ANTHROPIC_TEST_IMAGE_PATH="tests/assets/test_image_small.jpg"
export TEST_IMAGE_PATH_1="tests/assets/test_image_small.jpg"
export TEST_IMAGE_PATH_2="tests/assets/test_image_medium.png"

# Optional: Override default models
# export OPENAI_MODEL="gpt-4o-mini"
# export GEMINI_MODEL="gemini-2.0-flash"
# export ANTHROPIC_MODEL="claude-3-haiku-20240307"

echo "Environment configured for testing"
echo "Mock server PID: $MOCK_PID"
echo "Run 'make -C tests check' to execute tests"
echo "Run 'kill $MOCK_PID' to stop mock server when done"
```

## Running Tests

Once the environment is configured:

```bash
# Run all tests
make -C tests check

# Run a specific test
cd tests
./test_openai_text_dp

# Run tests with verbose output
make -C tests check VERBOSE=1
```

## Test Results

Tests will:
- **PASS** - Test completed successfully
- **SKIP** - Test skipped due to missing environment variables (exit code 77)
- **FAIL** - Test failed due to assertion or logic error
- **ERROR** - Test encountered an unexpected error

## Troubleshooting

### Tests are skipping
- Check that all required environment variables are set
- Verify API keys are valid and have appropriate permissions
- Ensure mock server is running and accessible

### Mock server tests failing
- Verify the mock server is running on the expected port
- Check that `DP_MOCK_SERVER` points to the correct URL
- Ensure Flask is installed and working

### Image-based tests failing
- Verify test image files exist in `tests/assets/`
- Check that image path environment variables point to valid files
- Ensure image files are readable

### API tests failing
- Verify API keys are valid and have sufficient quota
- Check network connectivity to API endpoints
- Review API service status pages for outages