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

    const char* api_key = getenv("GEMINI_API_KEY");
    if (!api_key) {
        fprintf(stderr, "Error: GEMINI_API_KEY environment variable not set.\n");
        curl_global_cleanup();
        return 1;
    }
    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Using Gemini API Key: ***\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context for Gemini.\n");
        curl_global_cleanup();
        return 1;
    }
    printf("Disaster Party Context Initialized.\n");

    dp_request_config_t request_config = {0};
    request_config.model = "gemini-1.5-flash-latest"; 
    request_config.temperature = 0.8;
    request_config.max_tokens = 200; 
    request_config.stream = false;

    dp_message_t messages[1];
    request_config.messages = messages;
    request_config.num_messages = 1;

    messages[0].role = DP_ROLE_USER;
    messages[0].num_parts = 0;
    messages[0].parts = NULL;
    if (!dp_message_add_text_part(&messages[0], "Write a short poem about coding in C with the Disaster Party library.")) {
        fprintf(stderr, "Failed to add text part to Gemini message.\n");
        dp_free_messages(messages, request_config.num_messages);
        dp_destroy_context(context);
        curl_global_cleanup();
        return 1;
    }
    
    printf("Sending request to Gemini model: %s\n", request_config.model);
    printf("Prompt: %s\n", messages[0].parts[0].text);

    dp_response_t response = {0};
    int result = dp_perform_completion(context, &request_config, &response);

    if (result == 0 && response.num_parts > 0 && response.parts[0].type == DP_CONTENT_PART_TEXT) {
        printf("\n--- Gemini Text Completion Response (HTTP %ld) ---\n", response.http_status_code);
        printf("%s\n", response.parts[0].text);
        if (response.finish_reason) printf("Finish Reason: %s\n", response.finish_reason);
        printf("-------------------------------------------\n");
    } else {
        fprintf(stderr, "\n--- Gemini Text Completion Failed (HTTP %ld) ---\n", response.http_status_code);
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
    printf("Gemini text test (Disaster Party) finished.\n");
    return result == 0 ? 0 : 1;
}

