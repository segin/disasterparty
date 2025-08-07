from flask import Flask, Response, request
import json

app = Flask(__name__)

# This mock server is specifically for Gemini 401 authentication failure tests.

@app.route('/v1/models/<model_id>:generateContent', methods=['POST'])
def completions_gemini(model_id):
    scenario = request.args.get('key')
    if scenario == 'AUTH_FAILURE_GEMINI':
        return Response(json.dumps({"error": {"message": "Invalid Authentication", "code": 401}}), status=401, mimetype='application/json')
    return Response(json.dumps({"error": "No test scenario triggered for Gemini completions endpoint"}), status=400, mimetype='application/json')

@app.route('/v1/models', methods=['GET'])
def list_models():
    scenario = request.args.get('key')
    if scenario == 'AUTH_FAILURE_GEMINI':
        return Response(json.dumps({"error": {"message": "Invalid Authentication", "code": 401}}), status=401, mimetype='application/json')
    return Response(json.dumps({"error": "No test scenario triggered for models endpoint"}), status=400, mimetype='application/json')

@app.route('/v1/files:upload', methods=['POST'])
def upload_file_gemini():
    scenario = request.args.get('key')
    if scenario == 'AUTH_FAILURE_GEMINI':
        return Response(json.dumps({"error": {"message": "Invalid Authentication", "code": 401}}), status=401, mimetype='application/json')
    return Response(json.dumps({"error": "No test scenario triggered for files:upload endpoint"}), status=400, mimetype='application/json')

@app.route('/v1/models/<model_id>:countTokens', methods=['POST'])
def count_tokens(model_id):
    scenario = request.args.get('key')
    if scenario == 'AUTH_FAILURE_GEMINI':
        return Response(json.dumps({"error": {"message": "Invalid Authentication", "code": 401}}), status=401, mimetype='application/json')
    return Response(json.dumps({"error": "No test scenario triggered for countTokens endpoint"}), status=400, mimetype='application/json')

if __name__ == '__main__':
    app.run(port=8080)