#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("Testing Anthropic multimodal token counting...\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_ANTHROPIC, "dummy_key", NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize context for Anthropic.\n");
        return EXIT_FAILURE;
    }

    dp_message_t message = {0};
    message.role = DP_ROLE_USER;
    dp_message_add_text_part(&message, "How many tokens are in this message?");
    dp_message_add_image_url_part(&message, "https://example.com/image1.png");
    dp_message_add_image_url_part(&message, "https://example.com/image2.png");

    dp_request_config_t request_config = {0};
    request_config.model = "claude-3-haiku-20240307";
    request_config.messages = &message;
    request_config.num_messages = 1;

    size_t token_count = 0;
    int ret = dp_count_tokens(context, &request_config, &token_count);

    if (ret != 0) {
        printf("SUCCESS: dp_count_tokens correctly failed with a dummy API key.\n");
        dp_free_messages(&message, 1);
        dp_destroy_context(context);
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "FAILURE: dp_count_tokens did not fail as expected.\n");
        fprintf(stderr, "Return code: %d, token count: %zu\n", ret, token_count);
        dp_free_messages(&message, 1);
        dp_destroy_context(context);
        return EXIT_FAILURE;
    }
}