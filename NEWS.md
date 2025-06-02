# Version 0.1.1 (2025-06-02)
* Updated library version to 0.1.1.
* Corrected SSE event separator detection for Gemini streams (handles `\r\n\r\n`).
* Updated OpenAI test model from `gpt-3.5-turbo` to `gpt-4.1-nano`.
* Updated Gemini test model to `gemini-2.0-flash` and token limits.
* Added `dp_list_models()` and `dp_free_model_list()` functions for enumerating models available at the API endpoint.
* Added man pages for all public functions and a library overview (section 7).
* Updated User-Agent string to `disasterparty/0.1.1`.
* Standardized bug reporting in man pages to GitHub issues.
* Minor internal fixes and debugging log cleanup.

# Version 0.1.0 (2025-05-01)
* Initial release of the Disaster Party LLM client library.
* Supports OpenAI-compatible and Google Gemini APIs.
* Features:
  * Text and multimodal (text + image URL/base64) inputs.
  * Non-streaming (full response) and streaming (token-by-token) completions.
  * Uses cJSON for robust JSON handling.
  * Uses libcurl for HTTP communication.
* Build system based on GNU Autotools.
* Renamed from an earlier prototype to "Disaster Party".
