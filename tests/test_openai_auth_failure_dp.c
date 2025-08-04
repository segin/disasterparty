#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_API_KEY "AUTH_FAILURE_OPENAI"
#define TEST_MODEL "gpt-3.5-turbo"

// Helper function to run a test case and check for 401
int run_auth_test(dp_context_t* context, const char* test_name) {
    printf("  Testing %s for 401...\n", test_name);
    int ret = -1;
    long http_status = 0;

    if (strcmp(test_name, "dp_perform_completion") == 0) {
        dp_request_config_t request_config = {0};
        request_config.model = TEST_MODEL;
        dp_message_t messages[1];
        memset(messages, 0, sizeof(messages));
        messages[0].role = DP_ROLE_USER;
        dp_message_add_text_part(&messages[0], "Test authentication failure for completion.");
        request_config.messages = messages;
        request_config.num_messages = 1;
        dp_response_t response = {0};
        ret = dp_perform_completion(context, &request_config, &response);
        http_status = response.http_status_code;
        dp_free_response_content(&response);
        dp_free_messages(messages, 1);
    } else if (strcmp(test_name, "dp_list_models") == 0) {
        dp_model_list_t* model_list = NULL;
        ret = dp_list_models(context, &model_list);
        http_status = model_list ? model_list->http_status_code : 0;
        dp_free_model_list(model_list);
    } else if (strcmp(test_name, "dp_count_tokens") == 0) {
        // OpenAI does not have a direct token counting endpoint in the current API
        // The function should return an error indicating this is not supported
        size_t token_count = 0;
        dp_request_config_t request_config = {0};
        request_config.model = TEST_MODEL;
        dp_message_t messages[1];
        memset(messages, 0, sizeof(messages));
        messages[0].role = DP_ROLE_USER;
        dp_message_add_text_part(&messages[0], "Test token counting for auth failure.");
        request_config.messages = messages;
        request_config.num_messages = 1;
        ret = dp_count_tokens(context, &request_config, &token_count);
        // For OpenAI, token counting may not be supported or may fail differently
        // We expect the function to return non-zero indicating failure/not supported
        if (ret != 0) {
            printf("  SUCCESS: dp_count_tokens correctly returned error for OpenAI (ret: %d).\n", ret);
            return 0;
        } else {
            fprintf(stderr, "  FAILURE: dp_count_tokens did not return error for OpenAI (ret: %d).\n", ret);
            return -1;
        }
        dp_free_messages(messages, 1);
    } else {
        fprintf(stderr, "  ERROR: Unknown test function %s\n", test_name);
        return -1;
    }

    if (ret != 0 && http_status == 401) {
        printf("  SUCCESS: %s correctly handled HTTP 401 (Unauthorized).\n", test_name);
        return 0;
    } else {
        fprintf(stderr, "  FAILURE: %s did not fail with HTTP 401 as expected (ret: %d, status: %ld).\n", test_name, ret, http_status);
        return -1;
    }
}

int main() {
    const char* mock_server_url = getenv("DP_MOCK_SERVER");
    if (!mock_server_url) {
        printf("SKIP: DP_MOCK_SERVER environment variable not set.\n");
        return 77;
    }

    printf("Testing OpenAI API authentication failure (HTTP 401)...\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, TEST_API_KEY, mock_server_url);
    if (!context) {
        fprintf(stderr, "Failed to initialize context for mock server.\n");
        return EXIT_FAILURE;
    }

    int overall_success = EXIT_SUCCESS;

    if (run_auth_test(context, "dp_perform_completion") != 0) overall_success = EXIT_FAILURE;
    if (run_auth_test(context, "dp_list_models") != 0) overall_success = EXIT_FAILURE;
    // Note: dp_upload_file function doesn't exist in current API, skipping this test
    if (run_auth_test(context, "dp_count_tokens") != 0) overall_success = EXIT_FAILURE;

    dp_destroy_context(context);

    return overall_success;
}