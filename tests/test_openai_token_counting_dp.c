#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
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
        printf("Successfully returned error for OpenAI token counting (as expected).\n");
    } else {
        fprintf(stderr, "Expected error for OpenAI token counting, but got result: %d\n", result);
    }

    dp_free_messages(messages, request_config.num_messages);
    dp_destroy_context(context);

    return (result == -1) ? EXIT_SUCCESS : EXIT_FAILURE;
}