#define _GNU_SOURCE
#include "disasterparty.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main() {
    const char* api_key = getenv("GEMINI_API_KEY");
    const char* base_url = getenv("GEMINI_API_BASE_URL");

    if (!api_key) {
        printf("SKIP: GEMINI_API_KEY not set.\n");
        return 77; // Automake's standard skip code
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed.\n");
        return EXIT_FAILURE;
    }

    const char* url_to_use = base_url ? base_url : "https://generativelanguage.googleapis.com/v1beta";

    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Using Gemini API Key: ***\n");
    printf("Using Gemini Base URL: %s\n", url_to_use);

    dp_context_t* context = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, api_key, url_to_use);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context.\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }

    dp_image_generation_config_t config = {0};
    config.prompt = "A cute robot holding a flower, watercolor style";
    config.model = "imagen-3.0-generate-001";
    config.n = 1;

    printf("Sending Image Generation Request (Imagen 3)...\n");
    dp_image_generation_response_t response = {0};
    int result = dp_generate_image(context, &config, &response);

    int exit_code = EXIT_FAILURE;
    if (result == 0 && response.http_status_code == 200 && response.num_images > 0) {
        printf("SUCCESS: Generated %zu image(s).\n", response.num_images);
        // Google usually returns base64 data
        if (response.images[0].base64_json) {
            printf("Base64 Data Length: %zu\n", strlen(response.images[0].base64_json));
        }
        if (response.images[0].url) {
            printf("URL: %s\n", response.images[0].url);
        }
        exit_code = EXIT_SUCCESS;
    } else {
        fprintf(stderr, "FAILURE: HTTP %ld, Error: %s\n", response.http_status_code, response.error_message);
        // Check for quota/billing errors to skip
        if (response.http_status_code == 429 || response.http_status_code == 403 ||
            (response.error_message && (
                strcasestr(response.error_message, "quota") || 
                strcasestr(response.error_message, "billing") || 
                strcasestr(response.error_message, "credit") ||
                strcasestr(response.error_message, "RESOURCE_EXHAUSTED")
            ))) {
            printf("SKIP: Billing or quota error detected.\n");
            exit_code = 77;
        }
    }

    dp_free_image_generation_response(&response);
    dp_destroy_context(context);
    curl_global_cleanup();

    return exit_code;
}