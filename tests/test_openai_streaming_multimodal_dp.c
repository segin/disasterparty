#include "disasterparty.h" 
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <stdbool.h>

int stream_handler(const char* token, void* user_data, bool is_final, const char* error_msg) {
    if (error_msg) {
        fprintf(stderr, "\nStream Error: %s\n", error_msg);
        return 1; 
    }
    if (token) {
        printf("%s", token);
        fflush(stdout); 
    }
    if (is_final && !error_msg) {
         printf("\n[STREAM END - OpenAI Multimodal]\n");
    }
    return 0; 
}

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
    printf("Testing OpenAI Streaming Multimodal:\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize context for OpenAI.\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }

    dp_request_config_t request_config = {0};
    request_config.model = "gpt-4o"; 
    request_config.temperature = 0.5;
    request_config.max_tokens = 300;
    request_config.stream = true; 

    dp_message_t messages[1];
    memset(messages, 0, sizeof(messages));
    request_config.messages = messages;
    request_config.num_messages = 1;

    messages[0].role = DP_ROLE_USER;

    if (!dp_message_add_text_part(&messages[0], "What is in this image? Stream your response.")) {
        fprintf(stderr, "Failed to add text part.\n");
        dp_destroy_context(context);
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    const char* image_url = "https://upload.wikimedia.org/wikipedia/commons/thumb/d/dd/Gfp-wisconsin-madison-the-nature-boardwalk.jpg/640px-Gfp-wisconsin-madison-the-nature-boardwalk.jpg";
    if (!dp_message_add_image_url_part(&messages[0], image_url)) {
        fprintf(stderr, "Failed to add image part.\n");
        dp_free_messages(messages, 1);
        dp_destroy_context(context);
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    
    printf("Sending streaming multimodal request to model: %s\n---\n", request_config.model);

    dp_response_t response_status = {0};
    int result = dp_perform_streaming_completion(context, &request_config, stream_handler, NULL, &response_status);
    
    printf("\n---\n"); 
    if (result == 0) {
        printf("OpenAI streaming multimodal test seems successful. HTTP: %ld\n", response_status.http_status_code);
    } else {
        fprintf(stderr, "OpenAI streaming multimodal test failed. HTTP: %ld, Error: %s\n", response_status.http_status_code, response_status.error_message ? response_status.error_message : "N/A");
    }

    bool success = (result == 0 && response_status.error_message == NULL && response_status.http_status_code == 200);
    int final_exit_code = success ? EXIT_SUCCESS : EXIT_FAILURE;

    dp_free_response_content(&response_status);
    dp_free_messages(messages, 1);
    dp_destroy_context(context);
    curl_global_cleanup();
    
    return final_exit_code;
}

