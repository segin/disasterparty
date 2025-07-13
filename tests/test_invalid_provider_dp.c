#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    dp_context_t* context = NULL;
    dp_response_t response = {0};
    int ret;
    int final_ret = EXIT_SUCCESS;

    printf("Testing invalid provider handling...\n");

    // Attempt to initialize a context with an invalid provider string
    context = dp_init_context(99, "dummy_key", NULL);
    if (context != NULL) {
        fprintf(stderr, "FAIL: dp_init_context succeeded with an invalid provider.\n");
        dp_destroy_context(context);
        return EXIT_FAILURE;
    }
    printf("PASS: dp_init_context correctly failed for an invalid provider.\n");

    // Initialize with a valid provider to test dp_perform_completion
    context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, "dummy_key", NULL);
    if (!context) {
        fprintf(stderr, "FAIL: Failed to initialize context for a valid provider.\n");
        return EXIT_FAILURE;
    }

    // Manually set an invalid provider in the context
    // This is a white-box test to simulate a corrupted context
    context->provider = 98;

    dp_request_config_t request_config = {0};
    request_config.model = "test-model";
    dp_message_t messages[1];
    memset(messages, 0, sizeof(messages));
    messages[0].role = DP_ROLE_USER;
    dp_message_add_text_part(&messages[0], "test");
    request_config.messages = messages;
    request_config.num_messages = 1;

    ret = dp_perform_completion(context, &request_config, &response);
    if (ret == 0) {
        fprintf(stderr, "FAIL: dp_perform_completion succeeded with an invalid provider in context.\n");
        final_ret = EXIT_FAILURE;
    } else {
        printf("PASS: dp_perform_completion correctly failed for an invalid provider.\n");
    }

    dp_free_response_content(&response);
    dp_free_messages(messages, 1);
    // Manually reset provider to avoid error in dp_destroy_context
    context->provider = DP_PROVIDER_OPENAI_COMPATIBLE;
    dp_destroy_context(context);

    return final_ret;
}
