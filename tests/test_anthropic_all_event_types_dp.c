#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <disasterparty.h>

static int test_count = 0;
static int test_failures = 0;

#define RUN_TEST(test_func) \
    do { \
        test_count++; \
        printf("Running %s...\n", #test_func); \
        if (test_func() != 0) { \
            test_failures++; \
            printf("FAIL: %s\n", #test_func); \
        } else { \
            printf("PASS: %s\n", #test_func); \
        } \
    } while (0)

static bool message_start_received = false;
static bool content_block_start_received = false;
static bool ping_received = false;
static bool content_block_delta_received = false;
static bool content_block_stop_received = false;
static bool message_delta_received = false;
static bool message_stop_received = false;
static bool error_received = false;

static int anthropic_event_callback(const dp_anthropic_stream_event_t* event, void* user_data, const char* error_during_stream) {
    (void)user_data;
    (void)error_during_stream;

    if (!event) return 0;

    switch (event->event_type) {
        case DP_ANTHROPIC_EVENT_MESSAGE_START:
            message_start_received = true;
            break;
        case DP_ANTHROPIC_EVENT_CONTENT_BLOCK_START:
            content_block_start_received = true;
            break;
        case DP_ANTHROPIC_EVENT_PING:
            ping_received = true;
            break;
        case DP_ANTHROPIC_EVENT_CONTENT_BLOCK_DELTA:
            content_block_delta_received = true;
            break;
        case DP_ANTHROPIC_EVENT_CONTENT_BLOCK_STOP:
            content_block_stop_received = true;
            break;
        case DP_ANTHROPIC_EVENT_MESSAGE_DELTA:
            message_delta_received = true;
            break;
        case DP_ANTHROPIC_EVENT_MESSAGE_STOP:
            message_stop_received = true;
            break;
        case DP_ANTHROPIC_EVENT_ERROR:
            error_received = true;
            break;
        default:
            break;
    }
    return 0;
}

static int test_anthropic_all_event_types() {
    dp_context_t* context = NULL;
    dp_request_config_t config = {0};
    dp_response_t response = {0};
    dp_message_t message = {0};
    int ret = -1;

    const char* api_key = getenv("ANTHROPIC_API_KEY");
    if (!api_key) {
        fprintf(stderr, "Skipping test_anthropic_all_event_types: ANTHROPIC_API_KEY not set.\n");
        return 0; // Skip test
    }

    context = dp_init_context(DP_PROVIDER_ANTHROPIC, api_key, NULL);
    assert(context != NULL);

    message.role = DP_ROLE_USER;
    assert(dp_message_add_text_part(&message, "Hello, tell me a short story."));

    config.model = "claude-3-opus-20240229"; // Or a suitable Anthropic model
    config.messages = &message;
    config.num_messages = 1;
    config.max_tokens = 100;
    config.temperature = 0.0;
    config.stream = true;

    ret = dp_perform_anthropic_streaming_completion(context, &config, anthropic_event_callback, NULL, &response);

    if (response.error_message) {
        fprintf(stderr, "API Error: %s\n", response.error_message);
    }

    assert(ret == 0);
    assert(response.http_status_code >= 200 && response.http_status_code < 300);

    // Assert that all expected event types were received
    assert(message_start_received);
    assert(content_block_start_received);
    assert(content_block_delta_received);
    assert(content_block_stop_received);
    assert(message_delta_received);
    assert(message_stop_received);
    // Ping and error events are not guaranteed for a successful completion

    dp_free_response_content(&response);
    dp_free_messages(&message, 1);
    dp_destroy_context(context);

    return 0;
}

int main() {
    printf("Starting Anthropic All Event Types Tests...\n");
    RUN_TEST(test_anthropic_all_event_types);
    printf("Anthropic All Event Types Tests finished.\n");

    if (test_failures > 0) {
        printf("Total tests run: %d, Failures: %d\n", test_count, test_failures);
        return 1;
    } else {
        printf("All %d tests passed.\n", test_count);
        return 0;
    }
}