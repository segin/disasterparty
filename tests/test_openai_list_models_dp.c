#include "disasterparty.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main() {
    const char* api_key = getenv("OPENAI_API_KEY");
    const char* base_url = getenv("OPENAI_API_BASE_URL");

    if (!api_key && !base_url) {
        printf("SKIP: OPENAI_API_KEY (and optionally OPENAI_API_BASE_URL) not set.\n");
        return 77;
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed.\n");
        return EXIT_FAILURE;
    }
    
    const char* key_to_use = api_key ? api_key : "ollama";

    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Testing OpenAI List Models:\n");
    printf("Using OpenAI API Key: ***\n");
    printf("Using OpenAI Base URL: %s\n", base_url ? base_url : "https://api.openai.com/v1");

    dp_context_t* context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, key_to_use, base_url);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context for OpenAI.\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    printf("Disaster Party Context Initialized.\n");

    dp_model_list_t* model_list = NULL;
    int result = dp_list_models(context, &model_list);

    if (result == 0 && model_list) {
        printf("\n--- OpenAI Available Models (HTTP %ld) ---\n", model_list->http_status_code);
        printf("Found %zu models:\n", model_list->count);
        for (size_t i = 0; i < model_list->count; ++i) {
            printf("  - ID: %s\n", model_list->models[i].model_id ? model_list->models[i].model_id : "(N/A)");
        }
        printf("-------------------------------------------\n");
    } else {
        fprintf(stderr, "\n--- OpenAI List Models Failed ---\n");
        if (model_list && model_list->error_message) {
            fprintf(stderr, "Error (HTTP %ld): %s\n", model_list->http_status_code, model_list->error_message);
        } else if (model_list) {
            fprintf(stderr, "Error (HTTP %ld): Unknown error from dp_list_models.\n", model_list->http_status_code);
        } else {
             fprintf(stderr, "Error: dp_list_models returned NULL for model_list structure.\n");
        }
        printf("-------------------------------------------\n");
    }

    bool success = (result == 0 && model_list && model_list->error_message == NULL && model_list->http_status_code == 200);
    int final_exit_code = success ? EXIT_SUCCESS : EXIT_FAILURE;

    dp_free_model_list(model_list);
    dp_destroy_context(context);
    
    curl_global_cleanup();
    printf("OpenAI list models test (Disaster Party) finished.\n");
    return final_exit_code;
}
