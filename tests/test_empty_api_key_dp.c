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

static int test_empty_api_key() {
    dp_context_t* context = NULL;
    dp_request_config_t config = {0};
    dp_response_t response = {0};
    dp_message_t message = {0};
    int ret = -1;

    // Test with empty API key for OpenAI
    context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, "", NULL);
    assert(context != NULL); // Should still create context

    message.role = DP_ROLE_USER;
    assert(dp_message_add_text_part(&message, "Hello."));

    config.model = "gpt-3.5-turbo";
    config.messages = &message;
    config.num_messages = 1;
    config.max_tokens = 10;
    config.temperature = 0.0;

    ret = dp_perform_completion(context, &config, &response);
    assert(ret == -1); // Should fail due to authentication
    assert(response.error_message != NULL);
    dp_free_response_content(&response);
    dp_free_messages(&message, 1);
    dp_destroy_context(context);

    // Test with empty API key for Gemini
    context = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, "", NULL);
    assert(context != NULL);

    message.role = DP_ROLE_USER;
    assert(dp_message_add_text_part(&message, "Hello."));

    config.model = "gemini-2.5-flash";
    config.messages = &message;
    config.num_messages = 1;
    config.max_tokens = 10;
    config.temperature = 0.0;

    ret = dp_perform_completion(context, &config, &response);
    assert(ret == -1);
    assert(response.error_message != NULL);
    dp_free_response_content(&response);
    dp_free_messages(&message, 1);
    dp_destroy_context(context);

    // Test with empty API key for Anthropic
    context = dp_init_context(DP_PROVIDER_ANTHROPIC, "", NULL);
    assert(context != NULL);

    message.role = DP_ROLE_USER;
    assert(dp_message_add_text_part(&message, "Hello."));

    config.model = "claude-3-haiku-20240307";
    config.messages = &message;
    config.num_messages = 1;
    config.max_tokens = 10;
    config.temperature = 0.0;

    ret = dp_perform_completion(context, &config, &response);
    assert(ret == -1);
    assert(response.error_message != NULL);
    dp_free_response_content(&response);
    dp_free_messages(&message, 1);
    dp_destroy_context(context);

    return 0;
}

int main() {
    printf("Starting Empty API Key Tests...\n");
    RUN_TEST(test_empty_api_key);
    printf("Empty API Key Tests finished.\n");

    if (test_failures > 0) {
        printf("Total tests run: %d, Failures: %d\n", test_count, test_failures);
        return 1;
    } else {
        printf("All %d tests passed.\n", test_count);
        return 0;
    }
}