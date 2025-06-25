#include "disasterparty.h" 
#include <curl/curl.h> 
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <stdbool.h>

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
        if (!error_msg) {
             printf("\n[STREAM END - OpenAI with Disaster Party]\n");
        }
    }
    return 0; 
}

int main() {
    const char* api_key = getenv("OPENAI_API_KEY");
    const char* base_url = getenv("OPENAI_API_BASE_URL");
    const char* model_env = getenv("OPENAI_MODEL");

    if (!api_key && !base_url) {
        printf("SKIP: OPENAI_API_KEY (and optionally OPENAI_API_BASE_URL) not set.\n");
        return 77;
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed.\n");
        return EXIT_FAILURE;
    }

    const char* model_to_use = model_env ? model_env : "gpt-4.1-nano";
    const char* key_to_use = api_key ? api_key : "ollama";

    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Using OpenAI API Key: ***\n");
    printf("Using OpenAI Base URL: %s\n", base_url ? base_url : "https://api.openai.com/v1");

    dp_context_t* context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, key_to_use, base_url);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context for OpenAI.\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    printf("Disaster Party Context Initialized for Streaming Test.\n");

    dp_request_config_t request_config = {0};
    request_config.model = model_to_use;
    request_config.temperature = 0.7;
    request_config.max_tokens = 300;
    request_config.stream = true; 

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
    
    printf("Sending streaming request to model: %s\n", request_config.model);
    printf("Prompt: %s\n---\nStreaming Response:\n", messages[0].parts[0].text);
    fflush(stdout);

    dp_response_t response_status = {0};
    
    int result = dp_perform_streaming_completion(context, &request_config, openai_stream_handler_dp, NULL, &response_status);

    printf("\n---\n"); 
    fflush(stdout);
    
    if (result == 0) { 
        printf("Streaming completed. HTTP Status: %ld\n", response_status.http_status_code);
        if (response_status.finish_reason) {
            printf("Finish Reason: %s\n", response_status.finish_reason);
        }
        if (response_status.error_message) { 
             fprintf(stderr, "[TEST_INFO] Overall operation reported an error: %s\n", response_status.error_message);
        }
    } else { 
        fprintf(stderr, "[TEST_INFO] Streaming request setup failed. HTTP Status: %ld\n", response_status.http_status_code);
        if (response_status.error_message) {
            fprintf(stderr, "[TEST_INFO] Error from library: %s\n", response_status.error_message);
        }
    }
    fflush(stderr);

    bool success = (result == 0 && response_status.error_message == NULL && response_status.http_status_code == 200);
    int final_exit_code = success ? EXIT_SUCCESS : EXIT_FAILURE;

    dp_free_response_content(&response_status);
    dp_free_messages(messages, request_config.num_messages);
    dp_destroy_context(context);
    curl_global_cleanup();
    printf("OpenAI streaming test (Disaster Party) finished.\n");
    return final_exit_code;
}
