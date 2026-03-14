#include "disasterparty.h" 
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <stdbool.h>

int detailed_cb(const dp_stream_event_t* event, void* user_data, const char* err) {
    if (err) {
        fprintf(stderr, "\nDetailed Stream Error: %s\n", err);
        return 1;
    }
    
    switch (event->event_type) {
        case DP_EVENT_MESSAGE_START: printf("[MESSAGE START]\n"); break;
        case DP_EVENT_CONTENT_BLOCK_START: printf("[CONTENT BLOCK START]\n"); break;
        case DP_EVENT_CONTENT_BLOCK_DELTA: 
            // Generic delta - for text, raw_json_data is the token string in simple providers
            // or the full SSE data in complex ones. The internal handler simplifies this for us.
            if (event->raw_json_data) printf("%s", event->raw_json_data);
            break;
        case DP_EVENT_THINKING_DELTA:
            if (event->raw_json_data) printf("<THINK>%s</THINK>", event->raw_json_data);
            break;
        case DP_EVENT_MESSAGE_STOP: printf("\n[MESSAGE STOP]\n"); break;
        default: break;
    }
    fflush(stdout);
    return 0;
}

void run_detailed_test(dp_provider_type_t provider, const char* api_key, const char* model) {
    if (!api_key) return;
    
    printf("\n--- Testing Detailed Streaming for Provider %d (%s) ---\n", provider, model);
    dp_context_t* ctx = dp_init_context(provider, api_key, NULL);
    dp_enable_advanced_features(ctx, DP_FEATURE_THINKING, 0);
    
    dp_message_t msg = { .role = DP_ROLE_USER };
    dp_message_add_text_part(&msg, "Think briefly and say HI.");
    
    dp_request_config_t config = {
        .model = model,
        .messages = &msg,
        .num_messages = 1,
        .stream = true
    };
    config.thinking.enabled = true;
    config.thinking.budget_tokens = 512;
    
    dp_response_t res = {0};
    dp_perform_detailed_streaming_completion(ctx, &config, detailed_cb, NULL, &res);
    
    dp_free_response_content(&res);
    dp_free_messages(&msg, 1);
    dp_destroy_context(ctx);
}

int main() {
    curl_global_init(CURL_GLOBAL_ALL);
    
    run_detailed_test(DP_PROVIDER_GOOGLE_GEMINI, getenv("GEMINI_API_KEY"), getenv("GEMINI_MODEL") ? getenv("GEMINI_MODEL") : "gemini-2.0-flash");
    run_detailed_test(DP_PROVIDER_ANTHROPIC, getenv("ANTHROPIC_API_KEY"), getenv("ANTHROPIC_MODEL") ? getenv("ANTHROPIC_MODEL") : "claude-3-7-sonnet-20250219");
    run_detailed_test(DP_PROVIDER_OPENAI_COMPATIBLE, getenv("OPENAI_API_KEY"), getenv("OPENAI_MODEL") ? getenv("OPENAI_MODEL") : "deepseek-reasoner");
    
    curl_global_cleanup();
    return 0;
}
