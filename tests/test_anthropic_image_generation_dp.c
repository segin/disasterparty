#define _GNU_SOURCE
#include "disasterparty.h"
#include "test_utils.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main() {
    load_env_file();
    // No valid API key needed for this test as it should fail locally before hitting the network
    const char* api_key = "dummy_key";
    const char* base_url = "https://api.anthropic.com/v1";

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed.\n");
        return EXIT_FAILURE;
    }

    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Testing Anthropic Image Generation Failure...\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_ANTHROPIC, api_key, base_url);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context.\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }

    dp_image_generation_config_t config = {0};
    config.prompt = "This should fail";
    
    dp_image_generation_response_t response = {0};
    int result = dp_generate_image(context, &config, &response);

    bool success = false;
    if (result == -1 && response.error_message != NULL) {
        if (strstr(response.error_message, "Provider not supported for image generation") != NULL) {
            printf("SUCCESS: Received expected error: %s\n", response.error_message);
            success = true;
        } else {
            fprintf(stderr, "FAILURE: Received unexpected error message: %s\n", response.error_message);
        }
    } else {
        fprintf(stderr, "FAILURE: Function returned success or no error message. Result: %d\n", result);
    }

    dp_free_image_generation_response(&response);
    dp_destroy_context(context);
    curl_global_cleanup();

    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}