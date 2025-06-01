#include "disaster_party.h" // Renamed
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

int gemini_stream_handler_dp(const char* token, void* user_data, bool is_final, const char* error_msg) { // Renamed
    if (error_msg) {
        fprintf(stderr, "\nStream Error: %s\n", error_msg);
        return 1; 
    }
    if (token) {
        printf("%s", token);
        fflush(stdout); 
    }
    if (is_final) {
        printf("\n[STREAM END - Gemini (SSE) with Disaster Party]\n");
    }
    return 0; 
}

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
    printf("Using Gemini API Key: ***\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, api_key, NULL); // Renamed
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context for Gemini.\n");
        curl_global_cleanup();
        return 1;
    }
    printf("Disaster Party Context Initialized for Streaming Test.\n");

    dp_request_config_t request_config = {0}; // Renamed
    request_config.model = "gemini-1.5-flash-latest"; 
    request_config.temperature = 0.5;
    request_config.max_tokens = 250; 
    // request_config.stream is implicitly true for this function with Gemini

    dp_message_t messages[1]; // Renamed
    request_config.messages = messages;
    request_config.num_messages = 1;

    messages[0].role = DP_ROLE_USER; // Renamed
    messages[0].num_parts = 0;
    messages[0].parts = NULL;
    if (!dp_message_add_text_part(&messages[0], "Explain the concept of recursion in programming as if you are teaching a beginner. Use a simple analogy. Stream your response using Disaster Party.")) { // Renamed
        fprintf(stderr, "Failed to add text part to Gemini message.\n");
        dp_free_messages(messages, request_config.num_messages); // Renamed
        dp_destroy_context(context); // Renamed
        curl_global_cleanup();
        return 1;
    }
    
    printf("Sending streaming request to Gemini model: %s\n", request_config.model);
    printf("Prompt: %s\n---\nStreaming Response (Gemini via SSE with Disaster Party):\n", messages[0].parts[0].text);

    dp_response_t response_status = {0}; // Renamed
    int result = dp_perform_streaming_completion(context, &request_config, gemini_stream_handler_dp, NULL, &response_status); // Renamed

    printf("\n---\n");
    if (result == 0) {
        printf("Gemini Streaming completed. HTTP Status: %ld\n", response_status.http_status_code);
         if (response_status.finish_reason) {
            printf("Finish Reason: %s\n", response_status.finish_reason);
        }
    } else {
        fprintf(stderr, "Gemini Streaming request failed. HTTP Status: %ld\n", response_status.http_status_code);
        if (response_status.error_message) {
            fprintf(stderr, "Error: %s\n", response_status.error_message);
        }
    }

    dp_free_response_content(&response_status); // Renamed
    dp_free_messages(messages, request_config.num_messages); // Renamed
    dp_destroy_context(context); // Renamed
    curl_global_cleanup();
    printf("Gemini streaming test (Disaster Party) finished.\n");
    return result == 0 ? 0 : 1;
}

