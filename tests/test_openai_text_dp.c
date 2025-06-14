#include "disasterparty.h" 
#include <curl/curl.h> 
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <stdbool.h>

int main() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed.\n");
        return EXIT_FAILURE;
    }

    const char* api_key = getenv("OPENAI_API_KEY");
    if (!api_key) {
        fprintf(stderr, "Error: OPENAI_API_KEY environment variable not set.\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }

    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Using OpenAI API Key: ***\n");
    printf("Using OpenAI Base URL: %s\n", getenv("OPENAI_API_BASE_URL") ? getenv("OPENAI_API_BASE_URL") : "https://api.openai.com/v1");

    dp_context_t* context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, api_key, getenv("OPENAI_API_BASE_URL"));
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context for OpenAI.\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    printf("Disaster Party Context Initialized.\n");

    dp_request_config_t request_config = {0};
    request_config.model = "gpt-4.1-nano";
    request_config.temperature = 0.7;
    request_config.max_tokens = 250;
    request_config.stream = false; 

    dp_message_t messages[2];
    memset(messages, 0, sizeof(messages));
    request_config.messages = messages;
    request_config.num_messages = 2;

    messages[0].role = DP_ROLE_SYSTEM;
    if (!dp_message_add_text_part(&messages[0], "You are a helpful assistant.")) {
        fprintf(stderr, "Failed to add text part to system message.\n");
        dp_destroy_context(context);
        curl_global_cleanup();
        return EXIT_FAILURE;
    }

    messages[1].role = DP_ROLE_USER;
    if (!dp_message_add_text_part(&messages[1], "Tell me about MAGIC GIANT.")) {
        fprintf(stderr, "Failed to add text part to user message.\n");
        dp_free_messages(messages, request_config.num_messages);
        dp_destroy_context(context);
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    
    printf("Sending request to model: %s\n", request_config.model);
    printf("Prompt: %s\n", messages[1].parts[0].text);

    dp_response_t response = {0};
    int result = dp_perform_completion(context, &request_config, &response);

    if (result == 0 && response.num_parts > 0 && response.parts[0].type == DP_CONTENT_PART_TEXT) {
        printf("\n--- OpenAI Text Completion Response (HTTP %ld) ---\n", response.http_status_code);
        printf("%s\n", response.parts[0].text);
        if (response.finish_reason) printf("Finish Reason: %s\n", response.finish_reason);
        printf("-------------------------------------------\n");
    } else {
        fprintf(stderr, "\n--- OpenAI Text Completion Failed (HTTP %ld) ---\n", response.http_status_code);
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
    printf("OpenAI text test (Disaster Party) finished.\n");
    return final_exit_code; 
}

