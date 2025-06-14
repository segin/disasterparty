#include "disasterparty.h"
#include <curl/curl.h>
#include <cjson/cJSON.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int anthropic_detailed_stream_handler(const dp_anthropic_stream_event_t* event, void* user_data, const char* error_msg) {
    (void)user_data; 

    if (error_msg) {
        fprintf(stderr, "\nStream Error reported by callback: %s\n", error_msg);
        fflush(stderr);
        return 1;
    }

    if (!event) {
        fprintf(stderr, "\nStream Error: Received NULL event pointer.\n");
        return 1;
    }

    const char* event_name = "UNKNOWN";
    switch(event->event_type) {
        case DP_ANTHROPIC_EVENT_MESSAGE_START: event_name = "message_start"; break;
        case DP_ANTHROPIC_EVENT_CONTENT_BLOCK_START: event_name = "content_block_start"; break;
        case DP_ANTHROPIC_EVENT_PING: event_name = "ping"; break;
        case DP_ANTHROPIC_EVENT_CONTENT_BLOCK_DELTA: event_name = "content_block_delta"; break;
        case DP_ANTHROPIC_EVENT_CONTENT_BLOCK_STOP: event_name = "content_block_stop"; break;
        case DP_ANTHROPIC_EVENT_MESSAGE_DELTA: event_name = "message_delta"; break;
        case DP_ANTHROPIC_EVENT_MESSAGE_STOP: event_name = "message_stop"; break;
        case DP_ANTHROPIC_EVENT_ERROR: event_name = "error"; break;
        default: break;
    }

    fprintf(stderr, "[EVENT: %s]\n", event_name);

    if (event->event_type == DP_ANTHROPIC_EVENT_CONTENT_BLOCK_DELTA && event->raw_json_data) {
        cJSON* root = cJSON_Parse(event->raw_json_data);
        if (root) {
            cJSON* delta = cJSON_GetObjectItemCaseSensitive(root, "delta");
            if (delta) {
                cJSON* text = cJSON_GetObjectItemCaseSensitive(delta, "text");
                if (cJSON_IsString(text) && text->valuestring) {
                    printf("%s", text->valuestring);
                    fflush(stdout);
                }
            }
            cJSON_Delete(root);
        }
    }
    
    if (event->event_type == DP_ANTHROPIC_EVENT_MESSAGE_STOP) {
        printf("\n[DETAILED STREAM END - Anthropic with Disaster Party]\n");
        fflush(stdout);
    }

    return 0; 
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    const char* api_key = getenv("ANTHROPIC_API_KEY");
    if (!api_key) {
        fprintf(stderr, "Error: ANTHROPIC_API_KEY environment variable not set.\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }

    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Testing Anthropic Detailed Streaming:\n");
    printf("Using Anthropic API Key: ***\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_ANTHROPIC, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize context for Anthropic.\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    printf("Disaster Party Context Initialized.\n");

    dp_request_config_t request_config = {0};
    request_config.model = "claude-3-haiku-20240307";
    request_config.temperature = 0.7;
    request_config.max_tokens = 300;
    request_config.stream = true;

    dp_message_t messages[1];
    memset(messages, 0, sizeof(messages));
    request_config.messages = messages;
    request_config.num_messages = 1;
    messages[0].role = DP_ROLE_USER;
    if (!dp_message_add_text_part(&messages[0], "Tell me about MAGIC GIANT.")) {
        fprintf(stderr, "Failed to add text part to user message.\n");
        dp_destroy_context(context);
        curl_global_cleanup();
        return EXIT_FAILURE;
    }

    printf("Sending detailed streaming request to model: %s\n", request_config.model);
    printf("Prompt: %s\n---\nStreaming Response:\n", messages[0].parts[0].text);
    fflush(stdout);

    dp_response_t response_status = {0};
    int result = dp_perform_anthropic_streaming_completion(context, &request_config, anthropic_detailed_stream_handler, NULL, &response_status);

    printf("\n---\n");
    fflush(stdout);

    if (result == 0) {
        printf("Anthropic Detailed Streaming completed. HTTP Status: %ld\n", response_status.http_status_code);
        if (response_status.finish_reason) {
            printf("Finish Reason: %s\n", response_status.finish_reason);
        }
        if (response_status.error_message) {
            fprintf(stderr, "[TEST_INFO] Overall operation reported an error by the library: %s\n", response_status.error_message);
        }
    } else {
        fprintf(stderr, "[TEST_INFO] Anthropic Detailed Streaming request setup failed. HTTP Status: %ld\n", response_status.http_status_code);
        if (response_status.error_message) {
            fprintf(stderr, "[TEST_INFO] Error from library: %s\n", response_status.error_message);
        }
    }
    fflush(stderr);

    bool success = (result == 0 && response_status.error_message == NULL && response_status.http_status_code == 200);
    int final_exit_code = success ? EXIT_SUCCESS : EXIT_FAILURE;

    dp_free_response_content(&response_status);
    dp_free_messages(messages, 1);
    dp_destroy_context(context);
    curl_global_cleanup();
    printf("Anthropic detailed streaming test (Disaster Party) finished.\n");
    return final_exit_code;
}

