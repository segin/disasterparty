#define _GNU_SOURCE
#include <string.h>
#include "disasterparty.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main() {
    const char* api_key = getenv("ANTHROPIC_API_KEY");
    const char* base_url = getenv("ANTHROPIC_API_BASE_URL");
    const char* model_env = getenv("ANTHROPIC_MODEL");

    if (!api_key) {
        printf("SKIP: ANTHROPIC_API_KEY not set.\n");
        return 77; // Automake's standard skip code
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed.\n");
        return EXIT_FAILURE;
    }

    // Thinking tokens are currently supported on specific models (e.g. Claude 3.7 Sonnet)
    const char* model_to_use = model_env ? model_env : "claude-3-7-sonnet-20250219"; 
    const char* url_to_use = base_url ? base_url : "https://api.anthropic.com/v1";

    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Using Anthropic API Key: ***\n");
    printf("Using Anthropic Base URL: %s\n", url_to_use);
    printf("Using Model: %s\n", model_to_use);

    dp_context_t* context = dp_init_context(DP_PROVIDER_ANTHROPIC, api_key, url_to_use);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context.\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }

    // --- Prepare Request ---
    dp_message_t messages[1];
    memset(messages, 0, sizeof(messages));

    messages[0].role = DP_ROLE_USER;
    if (!dp_message_add_text_part(&messages[0], "How many Rs are in the word strawberry? Explain your reasoning.")) {
        fprintf(stderr, "Failed to add text part.\n");
        return EXIT_FAILURE;
    }

    dp_request_config_t request_config = {0};
    request_config.model = model_to_use;
    request_config.messages = messages;
    request_config.num_messages = 1;
    request_config.max_tokens = 4000; // Increased to accommodate thinking + answer
    
    // Enable Thinking
    request_config.thinking.enabled = true;
    request_config.thinking.budget_tokens = 2000;

    printf("\n--- Sending Request with Thinking Enabled ---\n");
    dp_response_t response = {0};
    int result = dp_perform_completion(context, &request_config, &response);

    bool thinking_found = false;
    bool text_found = false;

    if (result == 0 && response.http_status_code == 200) {
        printf("Response received (HTTP 200). Parsing parts...\n");
        for (size_t i = 0; i < response.num_parts; ++i) {
            if (response.parts[i].type == DP_CONTENT_PART_THINKING) {
                printf("[Part %zu] Thinking:\n", i);
                printf("  Signature: %s\n", response.parts[i].thinking.signature ? response.parts[i].thinking.signature : "(null)");
                printf("  Content Length: %zu\n", response.parts[i].thinking.thinking ? strlen(response.parts[i].thinking.thinking) : 0);
                if (response.parts[i].thinking.thinking && strlen(response.parts[i].thinking.thinking) > 0) {
                    printf("  Snippet: %.100s...\n", response.parts[i].thinking.thinking);
                    thinking_found = true;
                }
            } else if (response.parts[i].type == DP_CONTENT_PART_TEXT) {
                printf("[Part %zu] Text: %.100s...\n", i, response.parts[i].text);
                text_found = true;
            } else {
                printf("[Part %zu] Unknown/Other Type: %d\n", i, response.parts[i].type);
            }
        }
    } else {
        fprintf(stderr, "Request failed. HTTP: %ld, Error: %s\n", response.http_status_code, response.error_message);
    }

    // Cleanup
    if (!success && (response.http_status_code == 429 || response.http_status_code == 402 || 
        (response.error_message && (
            strcasestr(response.error_message, "quota") || 
            strcasestr(response.error_message, "billing") ||
            strcasestr(response.error_message, "credit") ||
            strcasestr(response.error_message, "overloaded")
        )))) {
            printf("SKIP: Billing or quota error detected.\n");
            dp_free_response_content(&response);
            dp_destroy_context(context);
            curl_global_cleanup();
            return 77;
    }

    dp_free_response_content(&response);
    dp_free_messages(messages, 1);
    dp_destroy_context(context);
    curl_global_cleanup();

    if (thinking_found && text_found) {
        printf("\nSUCCESS: Received both thinking and text content.\n");
        return EXIT_SUCCESS;
    } else {
        if (!thinking_found) fprintf(stderr, "\nFAILURE: Did not receive thinking content.\n");
        if (!text_found) fprintf(stderr, "\nFAILURE: Did not receive text content.\n");
        return EXIT_FAILURE;
    }
}