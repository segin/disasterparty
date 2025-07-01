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

static int stop_callback_count = 0;
static int streaming_stop_callback(const char* token, void* user_data, bool is_final_chunk, const char* error_during_stream) {
    (void)user_data;
    (void)error_during_stream;

    if (token) {
        stop_callback_count++;
        // Stop after the first token
        return 1; 
    }
    return 0;
}

static int test_streaming_callback_stop() {
    dp_context_t* context = NULL;
    dp_request_config_t config = {0};
    dp_response_t response = {0};
    dp_message_t message = {0};
    int ret = -1;

    const char* api_key = getenv("OPENAI_API_KEY");
    if (!api_key) {
        fprintf(stderr, "Skipping test_streaming_callback_stop: OPENAI_API_KEY not set.\n");
        return 0; // Skip test
    }

    context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, api_key, NULL);
    assert(context != NULL);

    message.role = DP_ROLE_USER;
    assert(dp_message_add_text_part(&message, "Tell me a very long story."));

    config.model = "gpt-3.5-turbo";
    config.messages = &message;
    config.num_messages = 1;
    config.max_tokens = 200; // Request a long response
    config.stream = true;

    stop_callback_count = 0;
    ret = dp_perform_streaming_completion(context, &config, streaming_stop_callback, NULL, &response);

    printf("HTTP Status: %ld\n", response.http_status_code);
    if (response.error_message) {
        printf("Error: %s\n", response.error_message);
    }
    if (response.finish_reason) {
        printf("Finish Reason: %s\n", response.finish_reason);
    }

    assert(ret == 0); // Should complete successfully, even if aborted
    assert(response.http_status_code >= 200 && response.http_status_code < 300);
    assert(stop_callback_count == 1); // Should have received only one token before stopping

    dp_free_response_content(&response);
    dp_free_messages(&message, 1);
    dp_destroy_context(context);

    return 0;
}

int main() {
    printf("Starting Streaming Callback Stop Tests...\n");
    RUN_TEST(test_streaming_callback_stop);
    printf("Streaming Callback Stop Tests finished.\n");

    if (test_failures > 0) {
        printf("Total tests run: %d, Failures: %d\n", test_count, test_failures);
        return 1;
    } else {
        printf("All %d tests passed.\n", test_count);
        return 0;
    }
}