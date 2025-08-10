# Mock Server for Disaster Party Tests

This directory contains mock servers used for testing the Disaster Party library. The mock servers simulate various API scenarios including authentication failures, rate limiting, streaming errors, and other edge cases.

## Files

- `main.py` - Primary mock server that handles multiple providers and scenarios
- `gemini_auth_mock.py` - Specialized mock server for Gemini authentication testing

## Usage

### Running the Main Mock Server

#### Quick Start (Daemon Mode)
```bash
cd tests/mock-server
./start_server.sh
```

The server will start in daemon mode on `http://localhost:8080` by default.

#### Foreground Mode
```bash
cd tests/mock-server
./start_server.sh --foreground
```

#### Manual Start
```bash
cd tests/mock-server
python3 main.py [--port 8080] [--foreground]
```

### Server Management

#### Check Server Status
```bash
./control.py status
```

#### Restart Server (reload code changes)
```bash
./control.py restart
```

#### Stop Server
```bash
./control.py shutdown
# or
./stop_server.sh
```

### Running Tests with Mock Server

Set the `DP_MOCK_SERVER` environment variable to point to your mock server:

```bash
export DP_MOCK_SERVER=http://localhost:8080
make -C tests check
```

## Supported Test Scenarios

The mock server responds to different scenarios based on the API key or authorization header provided:

### Authentication Failures
- `AUTH_FAILURE_OPENAI` - Returns HTTP 401 for OpenAI endpoints
- `AUTH_FAILURE_GEMINI` - Returns HTTP 401 for Gemini endpoints  
- `AUTH_FAILURE_ANTHROPIC` - Returns HTTP 401 for Anthropic endpoints

### Streaming Scenarios
- `ABRUPT_STREAM` - Simulates abruptly terminated streaming connections
- `STREAM_ERROR_ANTHROPIC` - Simulates mid-stream error events for Anthropic
- `STREAM_PING_ANTHROPIC` - Simulates ping events in Anthropic streams

### Rate Limiting
- `RATE_LIMIT_COMPLETION` - Returns HTTP 429 for completion requests
- `RATE_LIMIT_LIST_MODELS` - Returns HTTP 429 for model listing requests

### File Upload Scenarios
- `ZERO_BYTE_FILE` - Tests handling of empty files
- `LARGE_FILE_UPLOAD` - Tests handling of files exceeding size limits

### Error Handling
- `NON_JSON_ERROR` - Returns non-JSON error responses
- `EMPTY_LIST` - Returns empty model lists

## API Endpoints

The mock server implements the following endpoints:

### OpenAI Compatible
- `POST /v1/chat/completions` - Chat completions
- `GET /v1/models` - List models
- `POST /v1/files` - File uploads

### Google Gemini
- `POST /v1/models/<model_id>:generateContent` - Generate content
- `POST /v1/files:upload` - File uploads
- `POST /v1/models/<model_id>:countTokens` - Token counting

### Anthropic
- `POST /v1/messages` - Messages endpoint
- `POST /v1/messages/count_tokens` - Token counting

### Management Endpoints
- `GET /_control/status` - Get server status and process information
- `POST /_control/restart` - Restart the server (reloads code changes)
- `POST /_control/shutdown` - Gracefully shutdown the server

## Server Features

### Daemonization
By default, the server runs as a daemon process in the background. Use `--foreground` to run in the foreground for debugging.

### Hot Restart
The server supports hot restart via the `/_control/restart` endpoint, which allows reloading code changes without manually stopping and starting the server.

### Process Management
The server properly handles signals and can be managed through the control endpoints or standard process signals.

## Dependencies

The mock server requires Flask:

```bash
pip3 install flask
```

## Test Integration

Tests that require the mock server will:
1. Check for the `DP_MOCK_SERVER` environment variable
2. Skip with exit code 77 if the variable is not set
3. Use specific API keys to trigger different mock scenarios
4. Validate the expected responses and error conditions

This allows tests to run in environments without the mock server while providing comprehensive testing when the server is available.