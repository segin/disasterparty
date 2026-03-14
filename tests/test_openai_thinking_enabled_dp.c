#include "disasterparty.h" 
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <stdbool.h>

int main() {
    const char* api_key = getenv("OPENAI_API_KEY");
    const char* base_url = getenv("OPENAI_API_BASE_URL");
    const char* model_env = getenv("OPENAI_MODEL");

    if (!api_key && !base_url) {
        printf("SKIP: OPENAI_API_KEY (and optionally OPENAI_API_BASE_URL) not set.\n");
        return 77;
    }

    curl_global_init(CURL_GLOBAL_ALL);

    // Default to a model known to support reasoning_content
    const char* model_to_use = model_env ? model_env : "gpt-5.4";
    const char* key_to_use = api_key ? api_key : "ollama";

    dp_context_t* context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, key_to_use, base_url);
    if (!context) {
        fprintf(stderr, "Failed to initialize context.\n");
        return 1;
    }

    // ENABLE THINKING FEATURE
    printf("Enabling thinking feature for OpenAI-compatible provider...\n");
    dp_enable_advanced_features(context, DP_FEATURE_THINKING, 0);

    dp_message_t* messages = calloc(1, sizeof(dp_message_t));
    messages[0].role = DP_ROLE_USER;
    dp_message_add_text_part(&messages[0], "How many Rs are in strawberry? Explain step by step.");

    dp_request_config_t request_config = {
        .model = model_to_use,
        .messages = messages,
        .num_messages = 1,
        .stream = true
    };

    printf("Sending streaming request to model: %s\n", model_to_use);
    printf("---\nStreaming Response:\n");

    dp_response_t response = {0};
    int content_tokens_count = 0;

    auto int stream_cb(const char* token, void* user_data, bool is_final, const char* err) {
        if (err) {
            fprintf(stderr, "\nStream Error: %s\n", err);
            return -1;
        }
        if (token) {
            // In interleaved mode, we just see the text.
            // If it's reasoning content delivered as tokens, we'll see it here.
            printf("%s", token);
            fflush(stdout);
            content_tokens_count++;
        }
        return 0;
    }

    int res = dp_perform_streaming_completion(context, &request_config, stream_cb, NULL, &response);

    printf("\n---\n");
    if (res == 0) {
        printf("OpenAI Streaming completed. HTTP %ld\n", response.http_status_code);
    } else {
        printf("OpenAI Streaming failed: %s\n", response.error_message ? response.error_message : "Unknown error");
    }

    dp_free_response_content(&response);
    dp_free_messages(messages, 1);
    free(messages);
    dp_destroy_context(context);
    curl_global_cleanup();
    
    return res == 0 ? 0 : 1;
}
