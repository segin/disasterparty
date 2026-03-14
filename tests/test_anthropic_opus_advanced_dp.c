#include "disasterparty.h"
#include "test_utils.h" 
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <stdbool.h>

int main() {
    load_env_file();
    const char* api_key = getenv("ANTHROPIC_API_KEY");
    const char* model_env = getenv("ANTHROPIC_MODEL");

    if (!api_key) {
        printf("SKIP: ANTHROPIC_API_KEY not set.\n");
        return 77;
    }

    curl_global_init(CURL_GLOBAL_ALL);

    const char* model_to_use = model_env ? model_env : "claude-4-6-opus-20260301";

    printf("Testing Anthropic Opus (Advanced Mode)...\n");
    dp_context_t* context = dp_init_context(DP_PROVIDER_ANTHROPIC, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize context.\n");
        return 1;
    }

    // ENABLE THINKING FEATURE
    dp_enable_advanced_features(context, DP_FEATURE_THINKING, 0);

    dp_message_t* messages = calloc(1, sizeof(dp_message_t));
    messages[0].role = DP_ROLE_USER;
    dp_message_add_text_part(&messages[0], "Analyze the structural integrity of a Dyson sphere around a red dwarf. Think very deeply.");

    dp_request_config_t request_config = {
        .model = model_to_use,
        .messages = messages,
        .num_messages = 1,
        .stream = true
    };
    request_config.thinking.enabled = true;
    request_config.thinking.budget_tokens = 4096;

    printf("Sending streaming request to model: %s\n", model_to_use);
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
        printf("Opus Streaming completed successfully.\n");
    } else {
        printf("Opus Streaming failed: %s\n", response.error_message ? response.error_message : "Unknown error");
    }

    dp_free_response_content(&response);
    dp_free_messages(messages, 1);
    free(messages);
    dp_destroy_context(context);
    curl_global_cleanup();
    
    return res == 0 ? 0 : 1;
}
