from flask import Flask, Response, request
import time
import json

app = Flask(__name__)

# This single endpoint will simulate different responses based on the prompt.
@app.route('/v1/chat/completions', methods=['POST'])
def completions_openai():
    data = request.get_json()
    scenario = None
    if 'Authorization' in request.headers:
        scenario = request.headers.get('Authorization').replace('Bearer ', '')
    elif 'x-api-key' in request.headers:
        scenario = request.headers.get('x-api-key')
    elif 'key' in request.args:
        scenario = request.args.get('key')

    # --- Scenario 1: Non-JSON Error ---
    if scenario == 'NON_JSON_ERROR':
        return Response('<html><body><h1>500 Internal Server Error</h1></body></html>', status=500, mimetype='text/html')

    # --- Scenario 2: Abruptly Terminated Stream ---
    if scenario == 'ABRUPT_STREAM':
        def generate_abrupt_stream():
            # Send a valid chunk first
            yield "data: " + json.dumps({"id":"chatcmpl-123","object":"chat.completion.chunk","created":1694268190,"model":"gpt-3.5-turbo-0613","choices":[{"index":0,"delta":{"role":"assistant"}},{"finish_reason":None}]}) + "\n\n"
            time.sleep(0.01)
            # Send a deliberately truncated JSON object. Note the missing closing braces and quotes.
            yield "data: {\"id\":\"truncated_chunk\", \"content\":\"partial"
        return Response(generate_abrupt_stream(), mimetype='text/event-stream')

    # --- Scenario 3: Rate Limit Exceeded ---
    if scenario == 'RATE_LIMIT_COMPLETION':
        return Response(json.dumps({"error": {"message": "Rate limit exceeded", "type": "rate_limit_error", "code": 429}}), status=429, mimetype='application/json')

    # --- Scenario 4: Authentication Failure (401) ---
    if scenario == 'AUTH_FAILURE_OPENAI':
        return Response(json.dumps({"error": {"message": "Invalid Authentication", "type": "invalid_request_error", "code": 401}}), status=401, mimetype='application/json')

    # --- Default: A valid, successful response (can be added later) ---
    return Response('{"error": "No test scenario triggered in mock server"}', status=400, mimetype='application/json')

@app.route('/v1/models/<model_id>:generateContent', methods=['POST'])
def completions_gemini(model_id):
    data = request.get_json()
    scenario = None
    if 'Authorization' in request.headers:
        scenario = request.headers.get('Authorization').replace('Bearer ', '')
    elif 'x-api-key' in request.headers:
        scenario = request.headers.get('x-api-key')
    elif 'key' in request.args:
        scenario = request.args.get('key')

    if scenario == 'AUTH_FAILURE_GEMINI':
        return Response(json.dumps({"error": {"message": "Invalid Authentication", "code": 401}}), status=401, mimetype='application/json')

    return Response(json.dumps({"error": "No test scenario triggered for Gemini completions endpoint"}), status=400, mimetype='application/json')

@app.route('/v1/messages', methods=['POST'])
def completions_anthropic():
    data = request.get_json()
    scenario = None
    if 'x-api-key' in request.headers:
        scenario = request.headers.get('x-api-key')

    if scenario == 'AUTH_FAILURE_ANTHROPIC':
        return Response(json.dumps({"type": "error", "message": "Authentication Error"}), status=401, mimetype='application/json')

    return Response(json.dumps({"error": "No test scenario triggered for Anthropic completions endpoint"}), status=400, mimetype='application/json')

@app.route('/v1/models', methods=['GET'])
def list_models():
    scenario = None
    if 'Authorization' in request.headers:
        scenario = request.headers.get('Authorization').replace('Bearer ', '')
    elif 'x-api-key' in request.headers:
        scenario = request.headers.get('x-api-key')
    elif 'key' in request.args:
        scenario = request.args.get('key')

    # --- Scenario: Empty Model List ---
    if scenario == 'EMPTY_LIST':
        return Response(json.dumps({"object": "list", "data": []}), mimetype='application/json')

    # --- Scenario: Rate Limit Exceeded ---
    if scenario == 'RATE_LIMIT_LIST_MODELS':
        return Response(json.dumps({"error": {"message": "Rate limit exceeded", "type": "rate_limit_error", "code": 429}}), status=429, mimetype='application/json')

    # --- Scenario: Authentication Failure (401) ---
    if scenario == 'AUTH_FAILURE_OPENAI' or scenario == 'AUTH_FAILURE_GEMINI' or scenario == 'AUTH_FAILURE_ANTHROPIC':
        return Response(json.dumps({"error": {"message": "Invalid Authentication", "type": "invalid_request_error", "code": 401}}), status=401, mimetype='application/json')

    # --- Default: A valid, successful response (can be added later) ---
    return Response('{"error": "No test scenario triggered for models endpoint"}', status=400, mimetype='application/json')


@app.route('/v1/files', methods=['POST'])
def upload_file_openai():
    scenario = None
    if 'Authorization' in request.headers:
        scenario = request.headers.get('Authorization').replace('Bearer ', '')
    elif 'x-api-key' in request.headers:
        scenario = request.headers.get('x-api-key')
    elif 'key' in request.args:
        scenario = request.args.get('key')

    if scenario == 'ZERO_BYTE_FILE':
        if request.content_length == 0:
            return Response(json.dumps({"error": {"message": "File is empty", "code": 400}}), status=400, mimetype='application/json')
        else:
            return Response(json.dumps({"id": "file-123", "filename": "test_file.txt", "purpose": "fine-tune", "bytes": request.content_length}), status=200, mimetype='application/json')
    elif scenario == 'LARGE_FILE_UPLOAD':
        # Simulate a provider size limit error
        if request.content_length > (100 * 1024 * 1024): # > 100MB
            return Response(json.dumps({"error": {"message": "File size exceeds limit", "code": 413}}), status=413, mimetype='application/json')
        else:
            return Response(json.dumps({"id": "file-456", "filename": "large_test_file.bin", "purpose": "fine-tune", "bytes": request.content_length}), status=200, mimetype='application/json')
    elif scenario == 'AUTH_FAILURE_OPENAI':
        return Response(json.dumps({"error": {"message": "Invalid Authentication", "type": "invalid_request_error", "code": 401}}), status=401, mimetype='application/json')

    return Response(json.dumps({"error": "No test scenario triggered for files endpoint"}), status=400, mimetype='application/json')

@app.route('/v1/files:upload', methods=['POST'])
def upload_file_gemini():
    scenario = None
    if 'Authorization' in request.headers:
        scenario = request.headers.get('Authorization').replace('Bearer ', '')
    elif 'x-api-key' in request.headers:
        scenario = request.headers.get('x-api-key')
    elif 'key' in request.args:
        scenario = request.args.get('key')

    if scenario == 'ZERO_BYTE_FILE':
        if request.content_length == 0:
            return Response(json.dumps({"error": {"message": "File is empty", "code": 400}}), status=400, mimetype='application/json')
        else:
            return Response(json.dumps({"id": "file-123", "filename": "test_file.txt", "purpose": "fine-tune", "bytes": request.content_length}), status=200, mimetype='application/json')
    elif scenario == 'LARGE_FILE_UPLOAD':
        # Simulate a provider size limit error
        if request.content_length > (100 * 1024 * 1024): # > 100MB
            return Response(json.dumps({"error": {"message": "File size exceeds limit", "code": 413}}), status=413, mimetype='application/json')
        else:
            return Response(json.dumps({"id": "file-456", "filename": "large_test_file.bin", "purpose": "fine-tune", "bytes": request.content_length}), status=200, mimetype='application/json')
    elif scenario == 'AUTH_FAILURE_GEMINI':
        return Response(json.dumps({"error": {"message": "Invalid Authentication", "code": 401}}), status=401, mimetype='application/json')

    return Response(json.dumps({"error": "No test scenario triggered for files:upload endpoint"}), status=400, mimetype='application/json')

@app.route('/v1/models/<model_id>:countTokens', methods=['POST'])
def count_tokens_gemini(model_id):
    scenario = None
    if 'Authorization' in request.headers:
        scenario = request.headers.get('Authorization').replace('Bearer ', '')
    elif 'x-api-key' in request.headers:
        scenario = request.headers.get('x-api-key')
    elif 'key' in request.args:
        scenario = request.args.get('key')

    if scenario == 'AUTH_FAILURE_GEMINI':
        return Response(json.dumps({"error": {"message": "Invalid Authentication", "code": 401}}), status=401, mimetype='application/json')

    return Response(json.dumps({"error": "No test scenario triggered for countTokens endpoint"}), status=400, mimetype='application/json')

@app.route('/v1/messages/count_tokens', methods=['POST'])
def count_tokens_anthropic():
    scenario = None
    if 'x-api-key' in request.headers:
        scenario = request.headers.get('x-api-key')

    if scenario == 'AUTH_FAILURE_ANTHROPIC':
        return Response(json.dumps({"type": "error", "message": "Authentication Error"}), status=401, mimetype='application/json')

    return Response(json.dumps({"error": "No test scenario triggered for Anthropic count_tokens endpoint"}), status=400, mimetype='application/json')

if __name__ == '__main__':
    app.run(port=8080)
