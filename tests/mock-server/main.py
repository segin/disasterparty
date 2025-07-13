from flask import Flask, Response, request
import time
import json

app = Flask(__name__)

# This single endpoint will simulate different responses based on the prompt.
@app.route('/v1/chat/completions', methods=['POST'])
def completions():
    data = request.get_json()
    scenario = request.headers.get('X-Test-Scenario')

    # --- Scenario 1: Non-JSON Error ---
    if scenario == 'NON_JSON_ERROR':
        return Response('<html><body><h1>500 Internal Server Error</h1></body></html>', status=500, mimetype='text/html')

    # --- Scenario 2: Abruptly Terminated Stream ---
    if scenario == 'ABRUPT_STREAM':
        def generate_abrupt_stream():
            # Send a valid chunk first
            yield "data: " + json.dumps({"id":"chatcmpl-123","object":"chat.completion.chunk","created":1694268190,"model":"gpt-3.5-turbo-0613","choices":[{"index":0,"delta":{"role":"assistant"},"finish_reason":None}]}) + "\n\n"
            time.sleep(0.01)
            # Send a deliberately truncated JSON object. Note the missing closing braces and quotes.
            yield "data: {\"id\":\"chatcmpl-123\",\"object\":\"chat.completion.chunk\",\"created\":1694268190,\"model\":\"gpt-3.5-turbo-0613\",\"choices\":[{\"index\":0,\"delta\":{\"content\":\", wo\""}
        return Response(generate_abrupt_stream(), mimetype='text/event-stream')

    # --- Default: A valid, successful response (can be added later) ---
    return Response('{"error": "No test scenario triggered in mock server"}', status=400, mimetype='application/json')

@app.route('/v1/models', methods=['GET'])
def list_models():
    # --- Scenario: Empty Model List ---
    if request.headers.get('X-Test-Scenario') == 'EMPTY_LIST':
        return Response(json.dumps({"object": "list", "data": []}), mimetype='application/json')

    # --- Default: A valid, successful response (can be added later) ---
    return Response('{"error": "No test scenario triggered for models endpoint"}', status=400, mimetype='application/json')


if __name__ == '__main__':
    app.run(port=8080)
