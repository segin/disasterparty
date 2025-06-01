#include "disasterparty.h" 
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

int main() {
    CURLcode global_init_res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (global_init_res != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed: %s\n", curl_easy_strerror(global_init_res));
        return 1;
    }

    const char* api_key = getenv("OPENAI_API_KEY");
    const char* base_url = getenv("OPENAI_API_BASE_URL"); 

    if (!api_key) {
        fprintf(stderr, "Error: OPENAI_API_KEY environment variable not set.\n");
        curl_global_cleanup();
        return 1;
    }

    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Using OpenAI API Key: ***\n");
    printf("Using OpenAI Base URL: %s\n", base_url ? base_url : "https://api.openai.com/v1");

    dp_context_t* context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, api_key, base_url);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context for OpenAI.\n");
        curl_global_cleanup();
        return 1;
    }
    printf("Disaster Party Context Initialized.\n");

    dp_request_config_t request_config = {0};
    request_config.model = "gpt-3.5-turbo"; 
    request_config.temperature = 0.7;
    request_config.max_tokens = 150;
    request_config.stream = false; 

    dp_message_t messages[2];
    request_config.messages = messages;
    request_config.num_messages = 2;

    messages[0].role = DP_ROLE_SYSTEM;
    messages[0].num_parts = 0; 
    messages[0].parts = NULL;
    if (!dp_message_add_text_part(&messages[0], "You are a helpful assistant.")) {
        fprintf(stderr, "Failed to add text part to system message.\n");
        dp_free_messages(messages, request_config.num_messages);
        dp_destroy_context(context);
        curl_global_cleanup();
        return 1;
    }

    messages[1].role = DP_ROLE_USER;
    messages[1].num_parts = 0; 
    messages[1].parts = NULL;
    if (!dp_message_add_text_part(&messages[1], "Hello! Can you tell me a short joke involving the phrase 'Disaster Party'?")) {
        fprintf(stderr, "Failed to add text part to user message.\n");
        dp_free_messages(messages, request_config.num_messages);
        dp_destroy_context(context);
        curl_global_cleanup();
        return 1;
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
        fprintf(stderr, "-------------------------------------------\n");
    }

    dp_free_response_content(&response);
    dp_free_messages(messages, request_config.num_messages); 
    dp_destroy_context(context);
    
    curl_global_cleanup();
    printf("OpenAI text test (Disaster Party) finished.\n");
    return result == 0 ? 0 : 1; 
}

