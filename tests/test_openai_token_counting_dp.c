#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    const char* api_key = "dummy_key";

    dp_context_t* context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context for OpenAI.\n");
        return EXIT_FAILURE;
    }

    dp_request_config_t request_config = {0};
    request_config.model = "gpt-3.5-turbo";

    dp_message_t messages[1];
    memset(messages, 0, sizeof(messages));
    request_config.messages = messages;
    request_config.num_messages = 1;

    messages[0].role = DP_ROLE_USER;
    dp_message_add_text_part(&messages[0], "This should fail because token counting is not supported for OpenAI.");

    size_t token_count = 0;
    long http_status_code = 0;
    int result = dp_count_tokens(context, &request_config, &token_count, &http_status_code);

    dp_free_messages(messages, request_config.num_messages);
    dp_destroy_context(context);

    if (result == DP_ERROR_TOKEN_COUNTING_NOT_SUPPORTED) {
        printf("PASS: dp_count_tokens correctly returned DP_ERROR_TOKEN_COUNTING_NOT_SUPPORTED for the OpenAI provider.\n");
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "FAIL: dp_count_tokens did not return DP_ERROR_TOKEN_COUNTING_NOT_SUPPORTED for the OpenAI provider. Result: %d\n", result);
        return EXIT_FAILURE;
    }
}