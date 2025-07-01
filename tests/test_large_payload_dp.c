#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <disasterparty.h>

#define API_KEY_ENV "OPENAI_API_KEY"
#define LARGE_TEXT_SIZE (1024 * 10) // 10 KB of text

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

static int test_large_payload() {
    dp_context_t* context = NULL;
    dp_request_config_t config = {0};
    dp_response_t response = {0};
    dp_message_t message = {0};
    int ret = -1;

    const char* api_key = getenv(API_KEY_ENV);
    if (!api_key) {
        fprintf(stderr, "Skipping test_large_payload: %s not set.\n", API_KEY_ENV);
        return 0; // Skip test
    }

    context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, api_key, NULL);
    assert(context != NULL);

    char* large_text = malloc(LARGE_TEXT_SIZE + 1);
    assert(large_text != NULL);
    memset(large_text, 'A', LARGE_TEXT_SIZE);
    large_text[LARGE_TEXT_SIZE] = '\0';

    message.role = DP_ROLE_USER;
    assert(dp_message_add_text_part(&message, large_text));

    config.model = "gpt-3.5-turbo";
    config.messages = &message;
    config.num_messages = 1;
    config.max_tokens = 50;
    config.temperature = 0.0;

    ret = dp_perform_completion(context, &config, &response);

    printf("HTTP Status: %ld\n", response.http_status_code);
    if (response.error_message) {
        printf("Error: %s\n", response.error_message);
    }
    if (response.finish_reason) {
        printf("Finish Reason: %s\n", response.finish_reason);
    }

    // Expecting success or a graceful error from the API if payload is too large
    assert(ret == 0 || (ret == -1 && response.http_status_code >= 400));

    dp_free_response_content(&response);
    dp_free_messages(&message, 1);
    dp_destroy_context(context);
    free(large_text);

    return 0;
}

int main() {
    printf("Starting Large Payload Tests...\n");
    RUN_TEST(test_large_payload);
    printf("Large Payload Tests finished.\n");

    if (test_failures > 0) {
        printf("Total tests run: %d, Failures: %d\n", test_count, test_failures);
        return 1;
    } else {
        printf("All %d tests passed.\n", test_count);
        return 0;
    }
}