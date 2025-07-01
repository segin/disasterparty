#include <stdio.h>
#include <stdlib.h>
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

static int test_null_or_empty_messages() {
    dp_context_t* context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, "dummy_key", NULL);
    assert(context != NULL);

    dp_request_config_t config = {0};
    dp_response_t response = {0};
    int ret;

    // Test dp_perform_completion with NULL messages
    config.model = "gpt-3.5-turbo";
    config.messages = NULL;
    config.num_messages = 0;
    config.stream = false;
    ret = dp_perform_completion(context, &config, &response);
    assert(ret == -1); // Should return error
    assert(response.error_message != NULL);
    dp_free_response_content(&response);

    // Test dp_perform_streaming_completion with NULL messages
    config.stream = true;
    ret = dp_perform_streaming_completion(context, &config, NULL, NULL, &response);
    assert(ret == -1); // Should return error
    assert(response.error_message != NULL);
    dp_free_response_content(&response);

    dp_destroy_context(context);
    return 0;
}

int main() {
    printf("Starting Null or Empty Messages Tests...\n");
    RUN_TEST(test_null_or_empty_messages);
    printf("Null or Empty Messages Tests finished.\n");

    if (test_failures > 0) {
        printf("Total tests run: %d, Failures: %d\n", test_count, test_failures);
        return 1;
    } else {
        printf("All %d tests passed.\n", test_count);
        return 0;
    }
}
