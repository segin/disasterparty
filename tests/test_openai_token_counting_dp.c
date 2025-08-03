#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Testing OpenAI token counting error cases...\n");
    
    int test_failures = 0;
    
    // Test 1: Basic error case - OpenAI provider should return error
    printf("\n=== Test 1: Basic OpenAI token counting error ===\n");
    
    // Use a dummy API key since we expect this to fail before making any network calls
    const char* api_key = "dummy_key";
    
    dp_context_t* context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context for OpenAI.\n");
        return EXIT_FAILURE;
    }

    const char* model_to_use = "gpt-3.5-turbo";

    dp_request_config_t request_config = {0};
    request_config.model = model_to_use;

    dp_message_t messages[1];
    memset(messages, 0, sizeof(messages));
    request_config.messages = messages;
    request_config.num_messages = 1;

    messages[0].role = DP_ROLE_USER;
    if (!dp_message_add_text_part(&messages[0], "Test message for token counting.")) {
        fprintf(stderr, "Failed to add text part to message.\n");
        dp_destroy_context(context);
        return EXIT_FAILURE;
    }

    size_t token_count = 0;
    int result = dp_count_tokens(context, &request_config, &token_count);

    // We expect this to fail with error code -1 for OpenAI provider
    if (result == -1) {
        printf("✓ Successfully returned error for OpenAI token counting (as expected)\n");
    } else {
        printf("✗ Expected error for OpenAI token counting, but got result: %d\n", result);
        test_failures++;
    }

    dp_free_messages(messages, request_config.num_messages);
    
    // Test 2: Error case with NULL parameters
    printf("\n=== Test 2: NULL parameter error cases ===\n");
    
    // Test with NULL context
    result = dp_count_tokens(NULL, &request_config, &token_count);
    if (result == -1) {
        printf("✓ Correctly rejected NULL context\n");
    } else {
        printf("✗ Should have rejected NULL context, got: %d\n", result);
        test_failures++;
    }
    
    // Test with NULL request config
    result = dp_count_tokens(context, NULL, &token_count);
    if (result == -1) {
        printf("✓ Correctly rejected NULL request config\n");
    } else {
        printf("✗ Should have rejected NULL request config, got: %d\n", result);
        test_failures++;
    }
    
    // Test with NULL output parameter
    result = dp_count_tokens(context, &request_config, NULL);
    if (result == -1) {
        printf("✓ Correctly rejected NULL output parameter\n");
    } else {
        printf("✗ Should have rejected NULL output parameter, got: %d\n", result);
        test_failures++;
    }
    
    // Test 3: Error case with invalid request config
    printf("\n=== Test 3: Invalid request config error cases ===\n");
    
    // Test with NULL model
    dp_request_config_t invalid_config = {0};
    invalid_config.model = NULL;
    invalid_config.messages = messages;
    invalid_config.num_messages = 1;
    
    result = dp_count_tokens(context, &invalid_config, &token_count);
    if (result == -1) {
        printf("✓ Correctly rejected NULL model\n");
    } else {
        printf("✗ Should have rejected NULL model, got: %d\n", result);
        test_failures++;
    }
    
    // Test with NULL messages
    invalid_config.model = model_to_use;
    invalid_config.messages = NULL;
    invalid_config.num_messages = 1;
    
    result = dp_count_tokens(context, &invalid_config, &token_count);
    if (result == -1) {
        printf("✓ Correctly rejected NULL messages\n");
    } else {
        printf("✗ Should have rejected NULL messages, got: %d\n", result);
        test_failures++;
    }
    
    // Test with zero messages
    invalid_config.messages = messages;
    invalid_config.num_messages = 0;
    
    result = dp_count_tokens(context, &invalid_config, &token_count);
    if (result == -1) {
        printf("✓ Correctly rejected zero messages\n");
    } else {
        printf("✗ Should have rejected zero messages, got: %d\n", result);
        test_failures++;
    }
    
    // Test 4: Different OpenAI models (all should fail)
    printf("\n=== Test 4: Different OpenAI models (all should fail) ===\n");
    
    const char* openai_models[] = {
        "gpt-4",
        "gpt-4-turbo",
        "gpt-3.5-turbo",
        "text-davinci-003"
    };
    
    for (size_t i = 0; i < sizeof(openai_models) / sizeof(openai_models[0]); i++) {
        request_config.model = openai_models[i];
        result = dp_count_tokens(context, &request_config, &token_count);
        if (result == -1) {
            printf("✓ Model %s correctly returned error\n", openai_models[i]);
        } else {
            printf("✗ Model %s should have returned error, got: %d\n", openai_models[i], result);
            test_failures++;
        }
    }
    
    dp_destroy_context(context);
    
    // Final results
    printf("\n=== Test Results ===\n");
    if (test_failures == 0) {
        printf("✓ All OpenAI token counting error tests passed!\n");
        printf("OpenAI token counting test (Disaster Party) finished successfully.\n");
        return EXIT_SUCCESS;
    } else {
        printf("✗ %d test(s) failed\n", test_failures);
        printf("OpenAI token counting test (Disaster Party) finished with failures.\n");
        return EXIT_FAILURE;
    }
}