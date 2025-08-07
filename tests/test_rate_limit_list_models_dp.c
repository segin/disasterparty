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

    printf("Testing dp_list_models with API rate limiting (HTTP 429)...\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, "RATE_LIMIT_LIST_MODELS", mock_server_url);
    if (!context) {
        fprintf(stderr, "Failed to initialize context for mock server.\n");
        return EXIT_FAILURE;
    }

    dp_model_list_t* model_list = NULL;
    int ret = dp_list_models(context, &model_list);

    if (ret != 0 && model_list && model_list->http_status_code == 429) {
        printf("SUCCESS: dp_list_models correctly handled HTTP 429 (Rate Limit Exceeded).\n");
        if (model_list->error_message) {
            printf("Error message: %s\n", model_list->error_message);
        }
        dp_free_model_list(model_list);
        dp_destroy_context(context);
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "FAILURE: dp_list_models did not fail with HTTP 429 as expected.\n");
        fprintf(stderr, "Return code: %d, HTTP status: %ld\n", ret, model_list ? model_list->http_status_code : 0);
        if (model_list && model_list->error_message) {
            fprintf(stderr, "Error message: %s\n", model_list->error_message);
        }
        dp_free_model_list(model_list);
        dp_destroy_context(context);
        return EXIT_FAILURE;
    }
}