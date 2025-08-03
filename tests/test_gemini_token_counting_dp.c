#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    const char* api_key = getenv("GEMINI_API_KEY");
    if (!api_key) {
        printf("SKIP: GEMINI_API_KEY environment variable not set.\n");
        return 77;
    }

    dp_context_t* context = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context for Gemini.\n");
        return EXIT_FAILURE;
    }

    const char* model_to_use = "gemini-1.5-flash";

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

    if (result == 0) {
        printf("Successfully counted tokens.\n");
        printf("Token count: %zu\n", token_count);
    } else {
        fprintf(stderr, "Failed to count tokens.\n");
    }

    dp_free_messages(messages, request_config.num_messages);
    dp_destroy_context(context);

    return (result == 0 && token_count > 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}