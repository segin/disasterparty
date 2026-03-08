#include "disasterparty.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed.\n");
        return EXIT_FAILURE;
    }

    printf("Testing Gemini Thinking Token Failure...\n");

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

    int passed_tests = 0;
    int failed_tests = 0;

    // --- Test: Thinking Tokens with Gemini (Completion) ---
    printf("Test: Thinking Tokens with Gemini (Completion)...\n");
    dp_context_t* ctx_gemini = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, "dummy_key", "http://localhost");
    if (!ctx_gemini) return EXIT_FAILURE;

    dp_response_t response = {0};
    int res = dp_perform_completion(ctx_gemini, &config, &response);

    if (res == -1) {
        if (response.error_message && strstr(response.error_message, "Thinking tokens are currently only supported by the Anthropic provider") != NULL) {
            printf("PASS\n");
            passed_tests++;
        } else {
            fprintf(stderr, "FAIL: Unexpected error message: %s\n", response.error_message ? response.error_message : "NULL");
            failed_tests++;
        }
    } else {
        fprintf(stderr, "FAIL: Expected failure for thinking tokens with Gemini, got success code.\n");
        failed_tests++;
    }
    
    dp_free_response_content(&response);
    dp_destroy_context(ctx_gemini);
    dp_free_messages(&msg, 1);
    curl_global_cleanup();

    if (failed_tests > 0) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}