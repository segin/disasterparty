#include "disasterparty.h"
#include "test_utils.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Dummy callback for Anthropic streaming
int anthropic_stream_callback(const dp_anthropic_stream_event_t* event, void* user_data, const char* error) {
    return 0;
}

int main() {
    load_env_file();
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed.\n");
        return EXIT_FAILURE;
    }

    printf("Testing OpenAI Context with Detailed Streaming (formerly Anthropic-only)...\n");

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
    config.stream = true;

    dp_context_t* ctx_openai = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, "dummy_key", "http://localhost:9999");
    if (!ctx_openai) {
        fprintf(stderr, "Failed to init context.\n");
        return EXIT_FAILURE;
    }

    dp_response_t response = {0};
    
    // Attempt to call Anthropic-specific function with OpenAI context
    int res = dp_perform_anthropic_streaming_completion(ctx_openai, &config, anthropic_stream_callback, NULL, &response);
    
    int result = EXIT_FAILURE;

    if (res == -1) {
        if (response.error_message && strstr(response.error_message, "curl_easy_perform() failed") != NULL) {
            printf("PASS: Provider check passed (got expected connection error).\n");
            result = EXIT_SUCCESS;
        } else {
            fprintf(stderr, "FAIL: Unexpected error message: %s\n", response.error_message ? response.error_message : "NULL");
        }
    } else {
        // If it somehow succeeded (unlikely with dummy key/url), that's also technically a pass for provider support
        printf("PASS: Request succeeded (unexpectedly).\n");
        result = EXIT_SUCCESS;
    }

    dp_free_response_content(&response);
    dp_destroy_context(ctx_openai);
    dp_free_messages(&msg, 1);
    curl_global_cleanup();

    return result;
}