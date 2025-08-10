#include "disasterparty.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main() {
    const char* mock_server_url = getenv("DP_MOCK_SERVER");
    if (!mock_server_url) {
        printf("SKIP: DP_MOCK_SERVER environment variable not set.\n");
        return 77;
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed.\n");
        return EXIT_FAILURE;
    }

    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Testing Empty Model List Scenario:\n");

    dp_context_t* context = NULL;
    context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, "EMPTY_LIST", mock_server_url);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context.\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    printf("Disaster Party Context Initialized.\n");

    dp_model_list_t* model_list = NULL;
    int result = dp_list_models(context, &model_list);

    bool success = false;
    if (result == 0 && model_list) {
        printf("\n--- Mock Server Response (HTTP %ld) ---\n", model_list->http_status_code);
        if (model_list->count == 0) {
            printf("Test PASSED: Received an empty list of models as expected.\n");
            success = true;
        } else {
            fprintf(stderr, "Test FAILED: Expected 0 models, but got %zu.\n", model_list->count);
        }
        printf("-------------------------------------------\n");
    } else {
        fprintf(stderr, "\n--- Test FAILED: dp_list_models call failed ---\n");
        if (model_list && model_list->error_message) {
            fprintf(stderr, "Error (HTTP %ld): %s\n", model_list->http_status_code, model_list->error_message);
        } else {
            fprintf(stderr, "Error: dp_list_models returned %d.\n", result);
        }
        printf("-------------------------------------------\n");
    }

    int final_exit_code = success ? EXIT_SUCCESS : EXIT_FAILURE;

    dp_free_model_list(model_list);
    dp_destroy_context(context);
    
    curl_global_cleanup();
    printf("Empty model list test finished.\n");
    return final_exit_code;
}