#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    const char* mock_server_url = getenv("DP_MOCK_SERVER");
    if (!mock_server_url) {
        printf("SKIP: DP_MOCK_SERVER environment variable not set.\n");
        return 77;
    }

    printf("Testing non-JSON error handling...\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, "NON_JSON_ERROR", mock_server_url);
    if (!context) {
        fprintf(stderr, "Failed to initialize context for mock server.\n");
        return EXIT_FAILURE;
    }

    dp_request_config_t request_config = {0};
    request_config.model = "test-model";
    dp_message_t messages[1];
    memset(messages, 0, sizeof(messages));
    messages[0].role = DP_ROLE_USER;
    dp_message_add_text_part(&messages[0], "test message");
    request_config.messages = messages;
    request_config.num_messages = 1;

    dp_response_t response = {0};
    int ret = dp_perform_completion(context, &request_config, &response);

    if (ret != 0 && response.http_status_code == 500) {
        printf("SUCCESS: dp_perform_completion correctly failed with a non-JSON error response.\n");
        dp_free_response_content(&response);
        dp_free_messages(messages, 1);
        dp_destroy_context(context);
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "FAILURE: dp_perform_completion did not fail as expected.\n");
        fprintf(stderr, "Return code: %d, HTTP status: %ld\n", ret, response.http_status_code);
        dp_free_response_content(&response);
        dp_free_messages(messages, 1);
        dp_destroy_context(context);
        return EXIT_FAILURE;
    }
}