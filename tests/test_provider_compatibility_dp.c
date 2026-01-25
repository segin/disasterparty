#include "disasterparty.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Dummy callback for streaming
int stream_callback(const char* token, void* user_data, bool is_final, const char* error) {
    return 0;
}

// Dummy callback for Anthropic streaming
int anthropic_stream_callback(const dp_anthropic_stream_event_t* event, void* user_data, const char* error) {
    return 0;
}

int main() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed.\n");
        return EXIT_FAILURE;
    }

    printf("Running Provider Compatibility Tests...\n");

    dp_message_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.role = DP_ROLE_USER;
    if (!dp_message_add_text_part(&msg, "Hi")) {
        return EXIT_FAILURE;
    }

    dp_request_config_t config = {0};
    config.model = "test-model";
    config.messages = &msg;
    config.num_messages = 1;
    config.thinking.enabled = true;
    config.thinking.budget_tokens = 1000;

    // --- Test 1: Thinking Tokens with OpenAI (Completion) ---
    printf("Test 1: Thinking Tokens with OpenAI (Completion)...\n");
    dp_context_t* ctx_openai = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, "dummy_key", "http://localhost");
    if (!ctx_openai) return EXIT_FAILURE;

    dp_response_t response = {0};
    int res = dp_perform_completion(ctx_openai, &config, &response);

    if (res != -1) {
        fprintf(stderr, "FAIL: Expected failure for thinking tokens with OpenAI, got success code.\n");
        return EXIT_FAILURE;
    }
    if (!response.error_message || strstr(response.error_message, "Thinking tokens are currently only supported by the Anthropic provider") == NULL) {
        fprintf(stderr, "FAIL: Unexpected error message: %s\n", response.error_message ? response.error_message : "NULL");
        return EXIT_FAILURE;
    }
    dp_free_response_content(&response);
    printf("PASS\n");

    // --- Test 2: Thinking Tokens with OpenAI (Streaming) ---
    printf("Test 2: Thinking Tokens with OpenAI (Streaming)...\n");
    config.stream = true;
    res = dp_perform_streaming_completion(ctx_openai, &config, stream_callback, NULL, &response);
     if (res != -1) {
        fprintf(stderr, "FAIL: Expected failure for thinking tokens with OpenAI streaming, got success code.\n");
        return EXIT_FAILURE;
    }
     if (!response.error_message || strstr(response.error_message, "Thinking tokens are currently only supported by the Anthropic provider") == NULL) {
        fprintf(stderr, "FAIL: Unexpected error message: %s\n", response.error_message ? response.error_message : "NULL");
        return EXIT_FAILURE;
    }
    dp_free_response_content(&response);
    printf("PASS\n");
    dp_destroy_context(ctx_openai);


    // --- Test 3: Thinking Tokens with Gemini ---
    printf("Test 3: Thinking Tokens with Gemini (Completion)...\n");
    dp_context_t* ctx_gemini = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, "dummy_key", "http://localhost");
    config.stream = false;
    memset(&response, 0, sizeof(response));

    res = dp_perform_completion(ctx_gemini, &config, &response);
    if (res != -1) {
        fprintf(stderr, "FAIL: Expected failure for thinking tokens with Gemini, got success code.\n");
        return EXIT_FAILURE;
    }
    if (!response.error_message || strstr(response.error_message, "Thinking tokens are currently only supported by the Anthropic provider") == NULL) {
        fprintf(stderr, "FAIL: Unexpected error message: %s\n", response.error_message ? response.error_message : "NULL");
        return EXIT_FAILURE;
    }
    dp_free_response_content(&response);
    printf("PASS\n");
    dp_destroy_context(ctx_gemini);

    // --- Test 4: Anthropic Specific Stream with OpenAI Context ---
    printf("Test 4: Anthropic Stream Function with OpenAI Context...\n");
    ctx_openai = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, "dummy_key", "http://localhost");
    memset(&response, 0, sizeof(response));
    
    // We expect this to fail validation before using callback
    res = dp_perform_anthropic_streaming_completion(ctx_openai, &config, anthropic_stream_callback, NULL, &response);
    
    if (res != -1) {
         fprintf(stderr, "FAIL: Expected failure for mismatched context/function, got success code.\n");
         return EXIT_FAILURE;
    }
    if (!response.error_message || strstr(response.error_message, "requires a context initialized with DP_PROVIDER_ANTHROPIC") == NULL) {
        fprintf(stderr, "FAIL: Unexpected error message: %s\n", response.error_message ? response.error_message : "NULL");
        return EXIT_FAILURE;
    }
    dp_free_response_content(&response);
    printf("PASS\n");
    dp_destroy_context(ctx_openai);

    dp_free_messages(&msg, 1);
    curl_global_cleanup();
    
    printf("All compatibility tests passed.\n");
    return EXIT_SUCCESS;
}