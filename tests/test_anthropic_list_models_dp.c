#include "disasterparty.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    const char* api_key = getenv("ANTHROPIC_API_KEY");
    if (!api_key) {
        fprintf(stderr, "Error: ANTHROPIC_API_KEY environment variable not set.\n");
        curl_global_cleanup();
        return 1;
    }

    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Testing Anthropic List Models:\n");
    printf("Using Anthropic API Key: ***\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_ANTHROPIC, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context for Anthropic.\n");
        curl_global_cleanup();
        return 1;
    }
    printf("Disaster Party Context Initialized.\n");

    dp_model_list_t* model_list = NULL;
    int result = dp_list_models(context, &model_list);

    if (result == 0 && model_list) {
        printf("\n--- Anthropic Available Models (HTTP %ld) ---\n", model_list->http_status_code);
        printf("Found %zu models:\n", model_list->count);
        for (size_t i = 0; i < model_list->count; ++i) {
            printf("  Model %zu:\n", i + 1);
            printf("    ID:           %s\n", model_list->models[i].model_id ? model_list->models[i].model_id : "(N/A)");
            printf("    Display Name: %s\n", model_list->models[i].display_name ? model_list->models[i].display_name : "(N/A)");
        }
        printf("-------------------------------------------\n");
    } else {
        fprintf(stderr, "\n--- Anthropic List Models Failed ---\n");
        if (model_list && model_list->error_message) {
            fprintf(stderr, "Error (HTTP %ld): %s\n", model_list->http_status_code, model_list->error_message);
        } else if (model_list) {
            fprintf(stderr, "Error (HTTP %ld): Unknown error from dp_list_models.\n", model_list->http_status_code);
        } else {
             fprintf(stderr, "Error: dp_list_models returned NULL for model_list structure.\n");
        }
        fprintf(stderr, "-------------------------------------------\n");
    }

    dp_free_model_list(model_list);
    dp_destroy_context(context);

    curl_global_cleanup();
    printf("Anthropic list models test (Disaster Party) finished.\n");
    return (result == 0 && model_list && model_list->error_message == NULL) ? 0 : 1;
}
