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
static bool error_event_received = false;
static bool message_stop_received = false;
static bool callback_error_message_present = false;
static bool unexpected_content_after_error = false;

// Custom stream handler for error event test
int anthropic_error_stream_handler(const dp_anthropic_stream_event_t* event, void* user_data, const char* error_msg) {
    
    (void)user_data; // Unused

    if (error_msg) {
        callback_error_message_present = true;
        fprintf(stderr, "\nStream Error reported by callback: %s\n", error_msg);
        // If an error message is present, we expect the stream to stop or be invalid.
        // No further content processing should occur.
        return 1; // Indicate that the stream should stop processing
    }
    if (!event) {
        fprintf(stderr, "\nStream Error: Received NULL event pointer.\n");
        return 1; // Indicate an error
    }

    switch(event->event_type) {
        case DP_ANTHROPIC_EVENT_MESSAGE_START:
            if (error_event_received) unexpected_content_after_error = true;
            message_start_received = true;
            break;
        case DP_ANTHROPIC_EVENT_CONTENT_BLOCK_START:
            if (error_event_received) unexpected_content_after_error = true;
            content_block_start_received = true;
            break;
        case DP_ANTHROPIC_EVENT_CONTENT_BLOCK_DELTA:
            if (error_event_received) unexpected_content_after_error = true;
            if (event->raw_json_data) {
                cJSON* root = cJSON_Parse(event->raw_json_data);
                if (root) {
                    cJSON* delta = cJSON_GetObjectItemCaseSensitive(root, "delta");
                    if (delta) {
                        cJSON* text = cJSON_GetObjectItemCaseSensitive(delta, "text");
                        if (cJSON_IsString(text) && text->valuestring) {
                            if (strcmp(text->valuestring, "Partial") == 0) {
                                first_delta_received = true;
                            } else {
                                fprintf(stderr, "Unexpected text delta: '%s'\n", text->valuestring);
                                unexpected_content_after_error = true;
                            }
                        }
                    }
                    cJSON_Delete(root);
                }
            }
            break;
        case DP_ANTHROPIC_EVENT_ERROR:
            error_event_received = true;
            // Check if the error message is as expected
            if (event->raw_json_data) {
                cJSON* root = cJSON_Parse(event->raw_json_data);
                if (root) {
                    cJSON* message = cJSON_GetObjectItemCaseSensitive(root, "message");
                    if (cJSON_IsString(message) && strcmp(message->valuestring, "Simulated mid-stream error.") == 0) {
                        // Expected error message
                    } else {
                        fprintf(stderr, "Unexpected error message in event: %s\n", event->raw_json_data);
                        unexpected_content_after_error = true;
                    }
                    cJSON_Delete(root);
                } else {
                    fprintf(stderr, "Failed to parse error event JSON: %s\n", event->raw_json_data);
                    unexpected_content_after_error = true;
                }
            } else {
                fprintf(stderr, "Error event with no data.\n");
                unexpected_content_after_error = true;
            }
            break;
        case DP_ANTHROPIC_EVENT_MESSAGE_STOP:
            if (error_event_received && !callback_error_message_present) {
                // If error event was received, message_stop might not come or might be unexpected
                // unless the library explicitly handles it after reporting the error.
                // For this test, we expect the stream to stop due to the error, so message_stop might not be relevant.
            }
            message_stop_received = true;
            break;
        case DP_ANTHROPIC_EVENT_PING:
            // Ping events should be ignored and not cause issues
            break;
        case DP_ANTHROPIC_EVENT_MESSAGE_DELTA:
            if (error_event_received) unexpected_content_after_error = true;
            break;
        case DP_ANTHROPIC_EVENT_CONTENT_BLOCK_STOP:
            if (error_event_received) unexpected_content_after_error = true;
            break;
        default:
            // Ignore other event types for this specific test
            break;
    }

    return 0; // Continue streaming unless an explicit error is returned
}

int main() {
    const char* mock_server_url = getenv("DP_MOCK_SERVER");
    if (!mock_server_url) {
        printf("SKIP: DP_MOCK_SERVER environment variable not set.\n");
        return 77;
    }

    printf("Testing Anthropic streaming for correct error event handling mid-stream...\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_ANTHROPIC, "STREAM_ERROR_ANTHROPIC", mock_server_url);
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
    if (!dp_message_add_text_part(&messages[0], "Say something that will trigger a mid-stream error.")) {
        fprintf(stderr, "Failed to add text part to user message.\n");
        dp_destroy_context(context);
        return EXIT_FAILURE;
    }

    dp_response_t response_status = {0};
    int result = dp_perform_anthropic_streaming_completion(context, &request_config, anthropic_error_stream_handler, NULL, &response_status);

    bool success = (result != 0 && response_status.error_message != NULL && response_status.http_status_code == 200 &&
                    message_start_received && content_block_start_received && first_delta_received &&
                    error_event_received && callback_error_message_present && !unexpected_content_after_error);

    if (success) {
        printf("  SUCCESS: Anthropic streaming error event handled correctly.\n");
    } else {
        fprintf(stderr, "  FAILURE: Anthropic streaming error event test failed.\n");
        fprintf(stderr, "    Result: %d, HTTP Status: %ld, Error Message: %s\n",
                result, response_status.http_status_code, response_status.error_message ? response_status.error_message : "(null)");
        fprintf(stderr, "    Flags: msg_start=%d, cb_start=%d, first_delta=%d, error_event=%d, callback_error_msg=%d, unexpected_content=%d\n",
                message_start_received, content_block_start_received, first_delta_received,
                error_event_received, callback_error_message_present, unexpected_content_after_error);
    }

    dp_free_response_content(&response_status);
    dp_free_messages(messages, 1);
    dp_destroy_context(context);

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}