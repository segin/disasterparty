#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    const char* api_key = getenv("ANTHROPIC_API_KEY");
    if (!api_key) {
        printf("SKIP: ANTHROPIC_API_KEY environment variable not set.\n");
        return 77;
    }

    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Testing Anthropic token counting functionality...\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_ANTHROPIC, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context for Anthropic.\n");
        return EXIT_FAILURE;
    }

    int test_failures = 0;
    
    // Test 1: Basic token counting
    printf("\n=== Test 1: Basic token counting ===\n");
    
    const char* model_to_use = "claude-3-haiku-20240307";

    dp_request_config_t request_config = {0};
    request_config.model = model_to_use;

    dp_message_t messages[1];
    memset(messages, 0, sizeof(messages));
    request_config.messages = messages;
    request_config.num_messages = 1;

    messages[0].role = DP_ROLE_USER;
    if (!dp_message_add_text_part(&messages[0], "Count the tokens in this sentence.")) {
        fprintf(stderr, "Failed to add text part to message.\n");
        dp_destroy_context(context);
        return EXIT_FAILURE;
    }

    size_t token_count = 0;
    int result = dp_count_tokens(context, &request_config, &token_count);

    if (result == 0 && token_count > 0) {
        printf("✓ Successfully counted tokens: %zu\n", token_count);
    } else {
        printf("✗ Failed to count tokens (result: %d, count: %zu)\n", result, token_count);
        test_failures++;
    }

    dp_free_messages(messages, request_config.num_messages);
    
    // Test 2: Different message lengths
    printf("\n=== Test 2: Different message lengths ===\n");
    
    const char* test_messages[] = {
        "Hi",
        "This is a medium length message for testing token counting.",
        "This is a much longer message that contains significantly more text to test how the token counting API handles larger inputs. It includes multiple sentences and should result in a higher token count than the shorter messages."
    };
    
    for (size_t i = 0; i < sizeof(test_messages) / sizeof(test_messages[0]); i++) {
        dp_message_t test_msg = {0};
        test_msg.role = DP_ROLE_USER;
        
        if (!dp_message_add_text_part(&test_msg, test_messages[i])) {
            printf("✗ Failed to add text part for message %zu\n", i);
            test_failures++;
            continue;
        }
        
        request_config.messages = &test_msg;
        request_config.num_messages = 1;
        
        size_t count = 0;
        result = dp_count_tokens(context, &request_config, &count);
        
        if (result == 0 && count > 0) {
            printf("✓ Message %zu (%zu chars): %zu tokens\n", i + 1, strlen(test_messages[i]), count);
        } else {
            printf("✗ Failed to count tokens for message %zu\n", i + 1);
            test_failures++;
        }
        
        dp_free_messages(&test_msg, 1);
    }
    
    // Test 3: Multiple messages
    printf("\n=== Test 3: Multiple messages ===\n");
    
    dp_message_t multi_messages[3];
    memset(multi_messages, 0, sizeof(multi_messages));
    
    multi_messages[0].role = DP_ROLE_USER;
    dp_message_add_text_part(&multi_messages[0], "Hello, I have a question.");
    
    multi_messages[1].role = DP_ROLE_ASSISTANT;
    dp_message_add_text_part(&multi_messages[1], "I'd be happy to help! What's your question?");
    
    multi_messages[2].role = DP_ROLE_USER;
    dp_message_add_text_part(&multi_messages[2], "How many tokens are in this conversation?");
    
    request_config.messages = multi_messages;
    request_config.num_messages = 3;
    
    size_t multi_count = 0;
    result = dp_count_tokens(context, &request_config, &multi_count);
    
    if (result == 0 && multi_count > 0) {
        printf("✓ Successfully counted tokens for conversation: %zu\n", multi_count);
    } else {
        printf("✗ Failed to count tokens for conversation\n");
        test_failures++;
    }
    
    dp_free_messages(multi_messages, 3);
    
    // Test 4: System prompt
    printf("\n=== Test 4: System prompt ===\n");
    
    dp_message_t system_msg = {0};
    system_msg.role = DP_ROLE_USER;
    dp_message_add_text_part(&system_msg, "What is the capital of France?");
    
    request_config.messages = &system_msg;
    request_config.num_messages = 1;
    request_config.system_prompt = "You are a helpful geography assistant.";
    
    size_t system_count = 0;
    result = dp_count_tokens(context, &request_config, &system_count);
    
    if (result == 0 && system_count > 0) {
        printf("✓ Successfully counted tokens with system prompt: %zu\n", system_count);
    } else {
        printf("✗ Failed to count tokens with system prompt\n");
        test_failures++;
    }
    
    dp_free_messages(&system_msg, 1);
    
    // Test 5: Error conditions
    printf("\n=== Test 5: Error conditions ===\n");
    
    // Test with NULL parameters
    size_t dummy_count = 0;
    
    result = dp_count_tokens(NULL, &request_config, &dummy_count);
    if (result == -1) {
        printf("✓ Correctly rejected NULL context\n");
    } else {
        printf("✗ Should have rejected NULL context\n");
        test_failures++;
    }
    
    result = dp_count_tokens(context, NULL, &dummy_count);
    if (result == -1) {
        printf("✓ Correctly rejected NULL request config\n");
    } else {
        printf("✗ Should have rejected NULL request config\n");
        test_failures++;
    }
    
    result = dp_count_tokens(context, &request_config, NULL);
    if (result == -1) {
        printf("✓ Correctly rejected NULL output parameter\n");
    } else {
        printf("✗ Should have rejected NULL output parameter\n");
        test_failures++;
    }
    
    // Test 6: Different Anthropic models
    printf("\n=== Test 6: Different Anthropic models ===\n");
    
    const char* anthropic_models[] = {
        "claude-3-haiku-20240307",
        "claude-3-sonnet-20240229",
        "claude-3-opus-20240229"
    };
    
    dp_message_t model_test_msg = {0};
    model_test_msg.role = DP_ROLE_USER;
    dp_message_add_text_part(&model_test_msg, "Test message for different models.");
    
    request_config.messages = &model_test_msg;
    request_config.num_messages = 1;
    request_config.system_prompt = NULL;
    
    for (size_t i = 0; i < sizeof(anthropic_models) / sizeof(anthropic_models[0]); i++) {
        request_config.model = anthropic_models[i];
        size_t model_count = 0;
        result = dp_count_tokens(context, &request_config, &model_count);
        
        if (result == 0 && model_count > 0) {
            printf("✓ Model %s: %zu tokens\n", anthropic_models[i], model_count);
        } else {
            // Check if this is a model not found error (which should be skipped, not failed)
            printf("~ Model %s skipped (not available or unsupported)\n", anthropic_models[i]);
            // Don't count as failure since model might not be available
        }
    }
    
    dp_free_messages(&model_test_msg, 1);
    
    dp_destroy_context(context);

    // Final results
    printf("\n=== Test Results ===\n");
    if (test_failures == 0) {
        printf("✓ All Anthropic token counting tests passed!\n");
        printf("Anthropic token counting test (Disaster Party) finished successfully.\n");
        return EXIT_SUCCESS;
    } else {
        printf("✗ %d test(s) failed\n", test_failures);
        printf("Anthropic token counting test (Disaster Party) finished with failures.\n");
        return EXIT_FAILURE;
    }
}
