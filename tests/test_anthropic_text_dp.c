#include "disasterparty.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main() {
    const char* api_key = getenv("ANTHROPIC_API_KEY");
    if (!api_key) {
        printf("SKIP: ANTHROPIC_API_KEY environment variable not set.\n");
        return 77;
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed.\n");
        return EXIT_FAILURE;
    }

    const char* model_env = getenv("ANTHROPIC_MODEL");
    const char* model_to_use = model_env ? model_env : "claude-3-haiku-20240307";

    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Using Anthropic API Key: ***\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_ANTHROPIC, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context for Anthropic.\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    printf("Disaster Party Context Initialized.\n");

    dp_request_config_t request_config = {0};
    request_config.model = model_to_use;
    request_config.temperature = 0.7;
    request_config.max_tokens = 512;
    request_config.stream = false;
    request_config.system_prompt = "You are a helpful and concise assistant.";

    dp_message_t messages[1];
    memset(messages, 0, sizeof(messages));
    request_config.messages = messages;
    request_config.num_messages = 1;

    messages[0].role = DP_ROLE_USER;
    if (!dp_message_add_text_part(&messages[0], "Tell me about MAGIC GIANT.")) {
        fprintf(stderr, "Failed to add text part to user message.\n");
        dp_destroy_context(context);
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    
    printf("Sending request to model: %s\n", request_config.model);
    printf("Prompt: %s\n", messages[0].parts[0].text);

    dp_response_t response = {0};
    int result = dp_perform_completion(context, &request_config, &response);

    if (result == 0 && response.num_parts > 0 && response.parts[0].type == DP_CONTENT_PART_TEXT) {
        printf("\n--- Anthropic Text Completion Response (HTTP %ld) ---\n", response.http_status_code);
        printf("%s\n", response.parts[0].text);
        if (response.finish_reason) printf("Finish Reason: %s\n", response.finish_reason);
        printf("-------------------------------------------\n");
    } else {
        fprintf(stderr, "\n--- Anthropic Text Completion Failed (HTTP %ld) ---\n", response.http_status_code);
        if (response.error_message) {
            fprintf(stderr, "Error: %s\n", response.error_message);
        } else {
            fprintf(stderr, "An unknown error occurred.\n");
        }
        printf("-------------------------------------------\n");
    }

    bool success = (result == 0 && response.error_message == NULL && response.http_status_code == 200);
    int final_exit_code = success ? EXIT_SUCCESS : EXIT_FAILURE;

    dp_free_response_content(&response);
    dp_free_messages(messages, request_config.num_messages); 
    dp_destroy_context(context);
    
    curl_global_cleanup();
    printf("Anthropic text test (Disaster Party) finished.\n");
    return final_exit_code;
}
