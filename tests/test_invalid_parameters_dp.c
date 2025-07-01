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

static int test_invalid_parameters() {
    dp_context_t* context = NULL;
    dp_request_config_t config = {0};
    dp_response_t response = {0};
    dp_message_t message = {0};
    int ret = -1;

    const char* api_key = getenv("OPENAI_API_KEY");
    if (!api_key) {
        fprintf(stderr, "Skipping test_invalid_parameters: OPENAI_API_KEY not set.\n");
        return 0; // Skip test
    }

    context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, api_key, NULL);
    assert(context != NULL);

    message.role = DP_ROLE_USER;
    assert(dp_message_add_text_part(&message, "Hello."));

    config.model = "gpt-3.5-turbo";
    config.messages = &message;
    config.num_messages = 1;
    config.max_tokens = 10;

    // Test with invalid temperature (negative)
    config.temperature = -0.5;
    printf("Testing negative temperature (%.1f)...\n", config.temperature);
    ret = dp_perform_completion(context, &config, &response);
    printf("  ret: %d, http_status_code: %ld, error_message: %s\n", ret, response.http_status_code, response.error_message ? response.error_message : "(null)");
    assert(ret == -1);
    assert(response.error_message != NULL);
    dp_free_response_content(&response);

    // Test with invalid temperature (too high for OpenAI, but library should pass it)
    config.temperature = 2.5;
    printf("Testing too high temperature (%.1f)...\n", config.temperature);
    ret = dp_perform_completion(context, &config, &response);
    printf("  ret: %d, http_status_code: %ld, error_message: %s\n", ret, response.http_status_code, response.error_message ? response.error_message : "(null)");
    // Expecting API error, not library error
    assert(ret == -1); 
    assert(response.http_status_code >= 400); 
    assert(response.error_message != NULL);
    dp_free_response_content(&response);

    // Test with invalid top_p (negative)
    config.temperature = 0.7; // Reset to valid
    config.top_p = -0.1;
    printf("Testing negative top_p (%.1f)...\n", config.top_p);
    ret = dp_perform_completion(context, &config, &response);
    printf("  ret: %d, http_status_code: %ld, error_message: %s\n", ret, response.http_status_code, response.error_message ? response.error_message : "(null)");
    assert(ret == -1);
    assert(response.error_message != NULL);
    dp_free_response_content(&response);

    // Test with invalid top_p (too high)
    config.top_p = 1.1;
    printf("Testing too high top_p (%.1f)...\n", config.top_p);
    ret = dp_perform_completion(context, &config, &response);
    printf("  ret: %d, http_status_code: %ld, error_message: %s\n", ret, response.http_status_code, response.error_message ? response.error_message : "(null)");
    assert(ret == -1);
    assert(response.error_message != NULL);
    dp_free_response_content(&response);

    dp_free_messages(&message, 1);
    dp_destroy_context(context);

    return 0;
}

int main() {
    printf("Starting Invalid Parameters Tests...\n");
    RUN_TEST(test_invalid_parameters);
    printf("Invalid Parameters Tests finished.\n");

    if (test_failures > 0) {
        printf("Total tests run: %d, Failures: %d\n", test_count, test_failures);
        return 1;
    } else {
        printf("All %d tests passed.\n", test_count);
        return 0;
    }
}