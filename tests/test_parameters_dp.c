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
        fprintf(stderr, "Skipping parameter test: OPENAI_API_KEY not set.\n");
        curl_global_cleanup();
        return 77; // Standard automake skip code
    }

    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Testing Advanced Parameter Submission (OpenAI):\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context.\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    printf("Disaster Party Context Initialized.\n");

    dp_request_config_t request_config = {0};
    request_config.model = "gpt-4.1-nano"; 
    request_config.temperature = 0.6;
    request_config.max_tokens = 50;
    
    // Set advanced parameters
    request_config.top_p = 0.9;
    const char* stop_seqs[] = {"\n", " Human:"};
    request_config.stop_sequences = stop_seqs;
    request_config.num_stop_sequences = 2;

    dp_message_t messages[1];
    memset(messages, 0, sizeof(messages));
    request_config.messages = messages;
    request_config.num_messages = 1;

    messages[0].role = DP_ROLE_USER;
    if (!dp_message_add_text_part(&messages[0], "Write a single sentence about space.")) {
        fprintf(stderr, "Failed to add text part to message.\n");
        dp_destroy_context(context);
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    
    printf("Sending request with advanced parameters...\n");

    dp_response_t response = {0};
    int result = dp_perform_completion(context, &request_config, &response);

    if (result == 0 && response.http_status_code == 200) {
        printf("PASS: API call with advanced parameters was successful.\n");
        printf("Response: %s\n", response.parts ? response.parts[0].text : "(no text)");
    } else {
        fprintf(stderr, "FAIL: API call with advanced parameters failed.\n");
        if (response.error_message) {
            fprintf(stderr, "Error (HTTP %ld): %s\n", response.http_status_code, response.error_message);
        }
    }

    bool success = (result == 0 && response.error_message == NULL && response.http_status_code == 200);
    int final_exit_code = success ? EXIT_SUCCESS : EXIT_FAILURE;

    dp_free_response_content(&response);
    dp_free_messages(messages, 1); 
    dp_destroy_context(context);
    curl_global_cleanup();
    
    return final_exit_code; 
}

