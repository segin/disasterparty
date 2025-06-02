#include "disasterparty.h" 
#include <curl/curl.h> 
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <stdbool.h> // For bool type

// Callback function to handle streamed tokens for Gemini
int gemini_stream_handler_dp(const char* token, void* user_data, bool is_final, const char* error_msg) {
    if (error_msg) {
        fprintf(stderr, "\nStream Error reported by callback: %s\n", error_msg);
        fflush(stderr);
        return 1; 
    }
    if (token) {
        printf("%s", token); 
        fflush(stdout); 
    }
    if (is_final) {
        if (!error_msg) { 
             printf("\n[STREAM END - Gemini (SSE) with Disaster Party]\n");
             fflush(stdout);
        }
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
    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Using Gemini API Key: ***\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context for Gemini.\n");
        curl_global_cleanup();
        return 1;
    }
    printf("Disaster Party Context Initialized for Streaming Test.\n");

    dp_request_config_t request_config = {0};
    request_config.model = "gemini-2.0-flash"; // User updated model
    request_config.temperature = 0.5;
    request_config.max_tokens = 2048; // User updated max_tokens
    
    dp_message_t messages[1];
    request_config.messages = messages;
    request_config.num_messages = 1;

    messages[0].role = DP_ROLE_USER;
    messages[0].num_parts = 0;
    messages[0].parts = NULL;
    if (!dp_message_add_text_part(&messages[0], "Tell me about MAGIC GIANT.")) { 
        fprintf(stderr, "Failed to add text part to Gemini message.\n");
        dp_free_messages(messages, request_config.num_messages);
        dp_destroy_context(context);
        curl_global_cleanup();
        return 1;
    }
    
    printf("Sending streaming request to Gemini model: %s\n", request_config.model);
    printf("Prompt: %s\n---\nStreaming Response (Gemini via SSE with Disaster Party):\n", messages[0].parts[0].text);
    fflush(stdout); 

    dp_response_t response_status = {0};
    int result = dp_perform_streaming_completion(context, &request_config, gemini_stream_handler_dp, NULL, &response_status);

    printf("\n---\n"); 
    fflush(stdout);

    if (result == 0) {
        printf("Gemini Streaming completed. HTTP Status: %ld\n", response_status.http_status_code);
         if (response_status.finish_reason) {
            printf("Finish Reason: %s\n", response_status.finish_reason);
        }
        if (response_status.error_message) {
             fprintf(stderr, "[TEST_INFO] Overall operation reported an error by the library: %s\n", response_status.error_message);
        }
    } else {
        fprintf(stderr, "[TEST_INFO] Gemini Streaming request setup failed. HTTP Status: %ld\n", response_status.http_status_code);
        if (response_status.error_message) {
            fprintf(stderr, "[TEST_INFO] Error from library: %s\n", response_status.error_message);
        }
    }
    fflush(stdout);
    fflush(stderr);

    // Debugging the exit condition
    bool cond1_result_is_0 = (result == 0);
    bool cond2_error_msg_is_null = (response_status.error_message == NULL);
    bool cond3_http_is_200 = (response_status.http_status_code == 200);

    fprintf(stderr, "[TEST_EXIT_COND_DEBUG_GEMINI] result==0: %d, error_msg==NULL: %d, http_status==200: %d\n", 
            cond1_result_is_0, cond2_error_msg_is_null, cond3_http_is_200);
    fflush(stderr);

    int final_exit_code = (cond1_result_is_0 && cond2_error_msg_is_null && cond3_http_is_200) ? 0 : 1;
    fprintf(stderr, "[TEST_EXIT_CODE_GEMINI] Final exit code: %d\n", final_exit_code);
    fflush(stderr);

    dp_free_response_content(&response_status);
    dp_free_messages(messages, request_config.num_messages);
    dp_destroy_context(context);
    curl_global_cleanup();
    printf("Gemini streaming test (Disaster Party) finished.\n");
    
    return final_exit_code;
}

