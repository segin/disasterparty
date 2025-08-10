from flask import Flask, Response, request, jsonify
import time
import json
import os
import sys
import signal
import subprocess
import threading
import atexit

app = Flask(__name__)

# Global variable to track if we should run in foreground
foreground_mode = False
server_process = None

# This single endpoint will simulate different responses based on the prompt.
@app.route('/v1/chat/completions', methods=['POST'])
@app.route('/chat/completions', methods=['POST'])
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
@app.route('/models/<model_id>:generateContent', methods=['POST'])
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
@app.route('/messages', methods=['POST'])
def completions_anthropic():
    data = request.get_json()
    scenario = None
    if 'x-api-key' in request.headers:
        scenario = request.headers.get('x-api-key')

    if scenario == 'AUTH_FAILURE_ANTHROPIC':
        return Response(json.dumps({"type": "error", "message": "Authentication Error"}), status=401, mimetype='application/json')
    elif scenario == 'STREAM_ERROR_ANTHROPIC':
        def generate_error_stream():
            yield "event: message_start\ndata: {\"message\":{\"id\":\"msg_01J1.1\",\"type\":\"message\",\"role\":\"assistant\",\"model\":\"claude-3-haiku-20240307\",\"stop_reason\":null,\"stop_sequence\":null,\"usage\":{\"input_tokens\":10,\"output_tokens\":1}}}\n\n"
            yield "event: content_block_start\ndata: {\"content_block\":{\"type\":\"text\",\"text\":\"\"},\"index\":0}\n\n"
            yield "event: content_block_delta\ndata: {\"delta\":{\"type\":\"text_delta\",\"text\":\"Partial\"},\"index\":0}\n\n"
            yield "event: error\ndata: {\"message\":\"Simulated mid-stream error.\"}\n\n"
        return Response(generate_error_stream(), mimetype='text/event-stream')
    elif scenario == 'STREAM_PING_ANTHROPIC':
        def generate_ping_stream():
            yield "event: message_start\ndata: {\"message\":{\"id\":\"msg_01J1.1\",\"type\":\"message\",\"role\":\"assistant\",\"model\":\"claude-3-opus-20240229\",\"stop_reason\":null,\"stop_sequence\":null,\"usage\":{\"input_tokens\":10,\"output_tokens\":1}}}\n\n"
            yield "event: content_block_start\ndata: {\"content_block\":{\"type\":\"text\",\"text\":\"\"},\"index\":0}\n\n"
            yield "event: content_block_delta\ndata: {\"delta\":{\"type\":\"text_delta\",\"text\":\"Hello\"},\"index\":0}\n\n"
            yield "event: ping\ndata: {}\n\n" # Ping event
            yield "event: content_block_delta\ndata: {\"delta\":{\"type\":\"text_delta\",\"text\":\" World!\"},\"index\":0}\n\n"
            yield "event: message_delta\ndata: {\"usage\":{\"output_tokens\":2},\"stop_reason\":\"end_turn\",\"stop_sequence\":null}\n\n"
            yield "event: message_stop\ndata: {}\n\n"
        return Response(generate_ping_stream(), mimetype='text/event-stream')

    return Response(json.dumps({"error": "No test scenario triggered for Anthropic completions endpoint"}), status=400, mimetype='application/json')

@app.route('/v1/models', methods=['GET'])
@app.route('/models', methods=['GET'])
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
    if scenario == 'AUTH_FAILURE_OPENAI' or scenario == 'AUTH_FAILURE_GEMINI':
        return Response(json.dumps({"error": {"message": "Invalid Authentication", "type": "invalid_request_error", "code": 401}}), status=401, mimetype='application/json')
    elif scenario == 'AUTH_FAILURE_ANTHROPIC':
        return Response(json.dumps({"type": "error", "message": "Authentication Error"}), status=401, mimetype='application/json')

    # --- Default: A valid, successful response (can be added later) ---
    return Response('{"error": "No test scenario triggered for models endpoint"}', status=400, mimetype='application/json')


@app.route('/v1/files', methods=['POST'])
@app.route('/files', methods=['POST'])
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
@app.route('/models/<model_id>:countTokens', methods=['POST'])
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
@app.route('/messages/count_tokens', methods=['POST'])
def count_tokens_anthropic():
    scenario = None
    if 'x-api-key' in request.headers:
        scenario = request.headers.get('x-api-key')

    if scenario == 'AUTH_FAILURE_ANTHROPIC':
        return Response(json.dumps({"type": "error", "message": "Authentication Error"}), status=401, mimetype='application/json')

    return Response(json.dumps({"error": "No test scenario triggered for Anthropic count_tokens endpoint"}), status=400, mimetype='application/json')

# Management endpoints
@app.route('/_control/restart', methods=['POST'])
def restart_server():
    """Restart the server by reloading the current script"""
    def restart():
        time.sleep(0.5)  # Give time for response to be sent
        print("Restarting server...")
        # Use os.execv to replace the current process
        os.execv(sys.executable, [sys.executable] + sys.argv)
    
    threading.Thread(target=restart).start()
    return jsonify({"status": "restarting", "message": "Server will restart in 0.5 seconds"})

@app.route('/_control/shutdown', methods=['POST'])
def shutdown_server():
    """Shutdown the server gracefully"""
    def shutdown():
        time.sleep(0.5)  # Give time for response to be sent
        print("Shutting down server...")
        os._exit(0)
    
    threading.Thread(target=shutdown).start()
    return jsonify({"status": "shutting_down", "message": "Server will shutdown in 0.5 seconds"})

@app.route('/_control/status', methods=['GET'])
def server_status():
    """Get server status information"""
    return jsonify({
        "status": "running",
        "version": "1.1.0",
        "pid": os.getpid(),
        "foreground_mode": foreground_mode,
        "endpoints": [
            "/_control/restart (POST) - Restart the server",
            "/_control/shutdown (POST) - Shutdown the server", 
            "/_control/status (GET) - Get server status"
        ]
    })

def daemonize():
    """Daemonize the current process"""
    try:
        # First fork
        pid = os.fork()
        if pid > 0:
            # Parent process, exit
            sys.exit(0)
    except OSError as e:
        print(f"Fork #1 failed: {e}", file=sys.stderr)
        sys.exit(1)

    # Decouple from parent environment
    os.chdir("/")
    os.setsid()
    os.umask(0)

    try:
        # Second fork
        pid = os.fork()
        if pid > 0:
            # Parent process, exit
            sys.exit(0)
    except OSError as e:
        print(f"Fork #2 failed: {e}", file=sys.stderr)
        sys.exit(1)

    # Redirect standard file descriptors
    sys.stdout.flush()
    sys.stderr.flush()
    
    # Redirect to /dev/null
    with open('/dev/null', 'r') as f:
        os.dup2(f.fileno(), sys.stdin.fileno())
    with open('/dev/null', 'w') as f:
        os.dup2(f.fileno(), sys.stdout.fileno())
        os.dup2(f.fileno(), sys.stderr.fileno())

def signal_handler(signum, frame):
    """Handle shutdown signals"""
    print(f"Received signal {signum}, shutting down...")
    os._exit(0)

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='Disaster Party Mock Server')
    parser.add_argument('--port', type=int, default=8080, help='Port to run the server on (default: 8080)')
    parser.add_argument('--foreground', action='store_true', help='Run in foreground mode (default: daemonize)')
    args = parser.parse_args()
    
    foreground_mode = args.foreground
    
    print(f"Starting Disaster Party mock server on port {args.port}")
    print(f"Set DP_MOCK_SERVER=http://localhost:{args.port} to use with tests")
    print(f"Management endpoints available at http://localhost:{args.port}/_control/")
    
    if not foreground_mode:
        print("Daemonizing server... (use --foreground to run in foreground)")
        daemonize()
    else:
        print("Running in foreground mode...")
    
    # Set up signal handlers
    signal.signal(signal.SIGTERM, signal_handler)
    signal.signal(signal.SIGINT, signal_handler)
    
    # Register cleanup function
    atexit.register(lambda: print("Server shutting down..."))
    
    try:
        app.run(host='0.0.0.0', port=args.port, debug=False, use_reloader=False)
    except KeyboardInterrupt:
        print("Server interrupted by user")
        sys.exit(0)
    except Exception as e:
        print(f"Server error: {e}")
        sys.exit(1)