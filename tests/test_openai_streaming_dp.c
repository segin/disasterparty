#include "disasterparty.h" 
#include <curl/curl.h> 
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 

int openai_stream_handler_dp(const char* token, void* user_data, bool is_final, const char* error_msg) {
    if (error_msg) {
        fprintf(stderr, "\nStream Error: %s\n", error_msg);
        return 1; 
    }
    if (token) {
        printf("%s", token);
        fflush(stdout); 
    }
    if (is_final) {
        // Only print stream end if no error message, as error implies abrupt end.
        if (!error_msg) {
             printf("\n[STREAM END - OpenAI with Disaster Party]\n");
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
    printf("Disaster Party Context Initialized for Streaming Test.\n");

    dp_request_config_t request_config = {0};
    request_config.model = "gpt-4.1-nano"; // Updated model name
    request_config.temperature = 0.7;
    request_config.max_tokens = 200;
    request_config.stream = true; 

    dp_message_t messages[1];
    request_config.messages = messages;
    request_config.num_messages = 1;

    messages[0].role = DP_ROLE_USER;
    messages[0].num_parts = 0; 
    messages[0].parts = NULL;
    if (!dp_message_add_text_part(&messages[0], "Tell me a very short story about a brave C programmer using the Disaster Party library. Stream your response.")) {
        fprintf(stderr, "Failed to add text part to user message.\n");
        dp_free_messages(messages, request_config.num_messages);
        dp_destroy_context(context);
        curl_global_cleanup();
        return 1;
    }
    
    printf("Sending streaming request to model: %s\n", request_config.model);
    printf("Prompt: %s\n---\nStreaming Response:\n", messages[0].parts[0].text);

    dp_response_t response_status = {0};
    
    int result = dp_perform_streaming_completion(context, &request_config, openai_stream_handler_dp, NULL, &response_status);

    // Newline after streaming output, before final status.
    // The callback handles printing the [STREAM END] message.
    // If no tokens were printed by the callback, ensure a newline is printed here.
    // For simplicity, let's assume the callback's final message provides enough separation.
    // We will just print a newline if one wasn't printed by the callback.
    // However, the callback *does* print a newline in its [STREAM END] message.
    printf("\n---\n"); 
    
    if (result == 0) { 
        printf("Streaming completed. HTTP Status: %ld\n", response_status.http_status_code);
        if (response_status.finish_reason) {
            printf("Finish Reason: %s\n", response_status.finish_reason);
        }
        // error_message in response_status for streaming indicates setup/transport errors
        // or unhandled errors propagated from the stream end.
        if (response_status.error_message) { 
             fprintf(stderr, "Overall operation reported an error: %s\n", response_status.error_message);
        }
    } else { 
        fprintf(stderr, "Streaming request setup failed. HTTP Status: %ld\n", response_status.http_status_code);
        if (response_status.error_message) {
            fprintf(stderr, "Error: %s\n", response_status.error_message);
        }
    }

    dp_free_response_content(&response_status);
    dp_free_messages(messages, request_config.num_messages);
    dp_destroy_context(context);
    curl_global_cleanup();
    printf("OpenAI streaming test (Disaster Party) finished.\n");
    return (result == 0 && response_status.error_message == NULL && response_status.http_status_code == 200) ? 0 : 1;
}

