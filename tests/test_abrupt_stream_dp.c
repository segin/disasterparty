#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static bool error_received = false;

int stream_handler(const char* token, void* user_data, bool is_final, const char* error_msg) {
    (void)user_data; // Unused
    (void)token;     // Unused
    (void)is_final;  // Unused
    
    if (error_msg) {
        error_received = true;
    }
    return 0;
}

int main() {
    const char* api_key = getenv("OPENAI_API_KEY");
    if (!api_key) {
        printf("SKIP: OPENAI_API_KEY not set.\n");
        return 77;
    }

    printf("Testing abrupt stream handling...\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize context for mock server.\n");
        return EXIT_FAILURE;
    }

    dp_request_config_t request_config = {0};
    request_config.model = "test-model";
    request_config.stream = true;
    
    dp_message_t messages[1];
    memset(messages, 0, sizeof(messages));
    messages[0].role = DP_ROLE_USER;
    if (!dp_message_add_text_part(&messages[0], "test message")) {
        fprintf(stderr, "Failed to add text part to message.\n");
        dp_destroy_context(context);
        return EXIT_FAILURE;
    }
    
    request_config.messages = messages;
    request_config.num_messages = 1;

    dp_response_t response = {0};
    int ret = dp_perform_streaming_completion(context, &request_config, stream_handler, NULL, &response);

    if (ret != 0 && error_received) {
        printf("SUCCESS: dp_perform_streaming_completion correctly failed with an abrupt stream.\n");
        dp_free_response_content(&response);
        dp_free_messages(messages, 1);
        dp_destroy_context(context);
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "FAILURE: dp_perform_streaming_completion did not fail as expected.\n");
        fprintf(stderr, "Return code: %d, error received: %d\n", ret, error_received);
        dp_free_response_content(&response);
        dp_free_messages(messages, 1);
        dp_destroy_context(context);
        return EXIT_FAILURE;
    }
}