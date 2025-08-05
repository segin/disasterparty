#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <cjson/cJSON.h>

// Global flags to track event reception
static bool message_start_received = false;
static bool content_block_start_received = false;
static bool first_delta_received = false;
static bool ping_received = false;
static bool second_delta_received = false;
static bool message_delta_received = false;
static bool message_stop_received = false;
static bool error_in_callback = false;

// Custom stream handler for ping test
int anthropic_ping_stream_handler(const dp_anthropic_stream_event_t* event, void* user_data, const char* error_msg) {
    (void)user_data; // Unused

    if (error_msg) {
        fprintf(stderr, "\nStream Error reported by callback: %s\n", error_msg);
        error_in_callback = true;
        return 1; // Indicate an error
    }
    if (!event) {
        fprintf(stderr, "\nStream Error: Received NULL event pointer.\n");
        error_in_callback = true;
        return 1; // Indicate an error
    }

    switch(event->event_type) {
        case DP_ANTHROPIC_EVENT_MESSAGE_START:
            message_start_received = true;
            break;
        case DP_ANTHROPIC_EVENT_CONTENT_BLOCK_START:
            content_block_start_received = true;
            break;
        case DP_ANTHROPIC_EVENT_CONTENT_BLOCK_DELTA:
            if (event->raw_json_data) {
                cJSON* root = cJSON_Parse(event->raw_json_data);
                if (root) {
                    cJSON* delta = cJSON_GetObjectItemCaseSensitive(root, "delta");
                    if (delta) {
                        cJSON* text = cJSON_GetObjectItemCaseSensitive(delta, "text");
                        if (cJSON_IsString(text) && text->valuestring) {
                            if (strcmp(text->valuestring, "Hello") == 0) {
                                first_delta_received = true;
                            } else if (strcmp(text->valuestring, " World!") == 0) {
                                second_delta_received = true;
                            } else {
                                fprintf(stderr, "Unexpected text delta: '%s'\n", text->valuestring);
                                error_in_callback = true;
                            }
                        }
                    }
                    cJSON_Delete(root);
                }
            }
            break;
        case DP_ANTHROPIC_EVENT_PING:
            ping_received = true;
            // Crucially, ensure no text is processed for ping events
            if (event->raw_json_data && strcmp(event->raw_json_data, "{}") != 0) {
                fprintf(stderr, "Ping event contained unexpected data: %s\n", event->raw_json_data);
                error_in_callback = true;
            }
            break;
        case DP_ANTHROPIC_EVENT_MESSAGE_DELTA:
            message_delta_received = true;
            break;
        case DP_ANTHROPIC_EVENT_MESSAGE_STOP:
            message_stop_received = true;
            break;
        case DP_ANTHROPIC_EVENT_ERROR:
            fprintf(stderr, "Received unexpected error event mid-stream.\n");
            error_in_callback = true;
            break;
        default:
            // Ignore other event types for this specific test
            break;
    }

    return 0; // Continue streaming
}

int main() {
    const char* mock_server_url = getenv("DP_MOCK_SERVER");
    if (!mock_server_url) {
        printf("SKIP: DP_MOCK_SERVER environment variable not set.\n");
        return 77;
    }

    printf("Testing Anthropic streaming for correct ping event handling...\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_ANTHROPIC, "STREAM_PING_ANTHROPIC", mock_server_url);
    if (!context) {
        fprintf(stderr, "Failed to initialize context for Anthropic.\n");
        return EXIT_FAILURE;
    }

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
    if (!dp_message_add_text_part(&messages[0], "Say Hello World.")) {
        fprintf(stderr, "Failed to add text part to user message.\n");
        dp_destroy_context(context);
        return EXIT_FAILURE;
    }

    dp_response_t response_status = {0};
    int result = dp_perform_anthropic_streaming_completion(context, &request_config, anthropic_ping_stream_handler, NULL, &response_status);

    bool success = (result == 0 && response_status.error_message == NULL && response_status.http_status_code == 200 &&
                    message_start_received && content_block_start_received && first_delta_received &&
                    ping_received && second_delta_received && message_delta_received &&
                    message_stop_received && !error_in_callback);

    if (success) {
        printf("  SUCCESS: Anthropic streaming ping event handled correctly.\n");
    } else {
        fprintf(stderr, "  FAILURE: Anthropic streaming ping event test failed.\n");
        fprintf(stderr, "    Result: %d, HTTP Status: %ld, Error Message: %s\n",
                result, response_status.http_status_code, response_status.error_message ? response_status.error_message : "(null)");
        fprintf(stderr, "    Flags: msg_start=%d, cb_start=%d, first_delta=%d, ping=%d, second_delta=%d, msg_delta=%d, msg_stop=%d, callback_error=%d\n",
                message_start_received, content_block_start_received, first_delta_received,
                ping_received, second_delta_received, message_delta_received, message_stop_received, error_in_callback);
    }

    dp_free_response_content(&response_status);
    dp_free_messages(messages, 1);
    dp_destroy_context(context);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}