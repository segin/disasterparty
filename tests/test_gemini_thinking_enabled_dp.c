#include "disasterparty.h"
#include "test_utils.h" 
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <stdbool.h>

int main() {
    load_env_file();
    const char* api_key = getenv("GEMINI_API_KEY");
    if (!api_key) {
        printf("GEMINI_API_KEY not set, skipping test.\n");
        return 0;
    }

    curl_global_init(CURL_GLOBAL_ALL);

    dp_context_t* context = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context.\n");
        return 1;
    }

    // ENABLE THINKING FEATURE
    printf("Enabling thinking feature...\n");
    dp_enable_advanced_features(context, DP_FEATURE_THINKING, 0);

    dp_message_t* messages = calloc(1, sizeof(dp_message_t));
    messages[0].role = DP_ROLE_USER;
    dp_message_add_text_part(&messages[0], "Think before you answer. Tell me about the band MAGIC GIANT.");

    dp_request_config_t request_config = {
        .model = getenv("GEMINI_MODEL") ? getenv("GEMINI_MODEL") : "gemini-flash-latest",
        .messages = messages,
        .num_messages = 1,
        .stream = true
    };

    // Add thinking config to payload
    request_config.thinking.enabled = true;
    request_config.thinking.budget_tokens = 1024;

    printf("Sending streaming request with thinking ENABLED to Gemini model: %s\n", request_config.model);
    printf("Thinking parts should be interleaved in the output.\n");
    printf("---\nStreaming Response:\n");

    dp_response_t response = {0};
    auto int stream_cb(const char* token, void* user_data, bool is_final, const char* err) {
        if (err) {
            fprintf(stderr, "\nStream Error: %s\n", err);
            return -1;
        }
        if (token) {
            printf("%s", token);
            fflush(stdout);
        }
        return 0;
    }

    int res = dp_perform_streaming_completion(context, &request_config, stream_cb, NULL, &response);

    printf("\n---\n");
    if (res == 0) {
        printf("Gemini Streaming completed successfully.\n");
    } else {
        printf("Gemini Streaming failed: %s\n", response.error_message ? response.error_message : "Unknown error");
    }

    dp_free_response_content(&response);
    dp_free_messages(messages, 1);
    free(messages);
    dp_destroy_context(context);
    curl_global_cleanup();
    
    return res == 0 ? 0 : 1;
}
