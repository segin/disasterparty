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
        fprintf(stderr, "Failed to initialize Disaster Party context for OpenAI (Multimodal).\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    printf("Disaster Party Context Initialized (Multimodal).\n");

    dp_request_config_t request_config = {0};
    request_config.model = "gpt-4o"; 
    request_config.temperature = 0.5;
    request_config.max_tokens = 300;
    request_config.stream = false;

    dp_message_t messages[1];
    memset(messages, 0, sizeof(messages));
    request_config.messages = messages;
    request_config.num_messages = 1;

    messages[0].role = DP_ROLE_USER;

    if (!dp_message_add_text_part(&messages[0], "What is in this image? Describe it in one sentence.")) {
        fprintf(stderr, "Failed to add text part to multimodal message.\n");
        dp_destroy_context(context);
        curl_global_cleanup();
        return EXIT_FAILURE;
    }

    const char* image_url = "https://upload.wikimedia.org/wikipedia/commons/thumb/d/dd/Gfp-wisconsin-madison-the-nature-boardwalk.jpg/640px-Gfp-wisconsin-madison-the-nature-boardwalk.jpg";
    if (!dp_message_add_image_url_part(&messages[0], image_url)) {
        fprintf(stderr, "Failed to add image URL part to multimodal message.\n");
        dp_free_messages(messages, request_config.num_messages);
        dp_destroy_context(context);
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    
    printf("Sending multimodal request to model: %s\n", request_config.model);
    printf("Text prompt: %s\n", messages[0].parts[0].text);
    printf("Image URL: %s\n", messages[0].parts[1].image_url);


    dp_response_t response = {0};
    int result = dp_perform_completion(context, &request_config, &response);

    if (result == 0 && response.num_parts > 0 && response.parts[0].type == DP_CONTENT_PART_TEXT) {
        printf("\n--- OpenAI Multimodal Completion Response (HTTP %ld) ---\n", response.http_status_code);
        printf("%s\n", response.parts[0].text);
        if (response.finish_reason) printf("Finish Reason: %s\n", response.finish_reason);
        printf("---------------------------------------------------\n");
    } else {
        fprintf(stderr, "\n--- OpenAI Multimodal Completion Failed (HTTP %ld) ---\n", response.http_status_code);
        if (response.error_message) {
            fprintf(stderr, "Error: %s\n", response.error_message);
        } else {
            fprintf(stderr, "An unknown error occurred.\n");
        }
        printf("---------------------------------------------------\n");
    }

    bool success = (result == 0 && response.error_message == NULL && response.http_status_code == 200);
    int final_exit_code = success ? EXIT_SUCCESS : EXIT_FAILURE;

    dp_free_response_content(&response);
    dp_free_messages(messages, request_config.num_messages);
    dp_destroy_context(context);
    curl_global_cleanup();
    printf("OpenAI multimodal test (Disaster Party) finished.\n");
    return final_exit_code;
}

