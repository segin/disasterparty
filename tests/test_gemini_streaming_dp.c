#include "disasterparty.h" 
#include <curl/curl.h> 
#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

// Callback function to handle streamed tokens for Gemini
int gemini_stream_handler_dp(const char* token, void* user_data, bool is_final, const char* error_msg) {
    // ---- START TEST CALLBACK DEBUGGING ----
    fprintf(stderr, "[TEST_CALLBACK_GEMINI] Entered gemini_stream_handler_dp.\n");
    if (token) {
        fprintf(stderr, "[TEST_CALLBACK_GEMINI] Received token: <%s>\n", token);
    } else {
        fprintf(stderr, "[TEST_CALLBACK_GEMINI] Received NULL token.\n");
    }
    fprintf(stderr, "[TEST_CALLBACK_GEMINI] is_final: %s\n", is_final ? "true" : "false");
    if (error_msg) {
        fprintf(stderr, "[TEST_CALLBACK_GEMINI] Received error_msg: <%s>\n", error_msg);
    } else {
        fprintf(stderr, "[TEST_CALLBACK_GEMINI] Received NULL error_msg.\n");
    }
    fflush(stderr);
    // ---- END TEST CALLBACK DEBUGGING ----

    if (error_msg) {
        // Don't print to stdout if there's a stream error, it's already logged to stderr
        // printf("\nStream Error: %s\n", error_msg); // Already handled by stderr log
        return 1; // Signal to stop
    }
    if (token) {
        printf("%s", token); // Print to stdout as intended
        fflush(stdout); 
    }
    if (is_final) {
        // Only print stream end to stdout if no error message, as error implies abrupt end.
        if (!error_msg) {
             printf("\n[STREAM END - Gemini (SSE) with Disaster Party]\n");
             fflush(stdout);
        }
    }
    return 0; // Continue streaming
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
    request_config.model = "gemini-1.5-flash-latest"; 
    request_config.temperature = 0.5;
    request_config.max_tokens = 250; 
    // request_config.stream is not explicitly used by Gemini payload builder for this library,
    // as dp_perform_streaming_completion uses the streaming endpoint for Gemini.

    dp_message_t messages[1];
    request_config.messages = messages;
    request_config.num_messages = 1;

    messages[0].role = DP_ROLE_USER;
    messages[0].num_parts = 0;
    messages[0].parts = NULL;
    if (!dp_message_add_text_part(&messages[0], "Explain the concept of recursion in programming as if you are teaching a beginner. Use a simple analogy. Stream your response using Disaster Party.")) {
        fprintf(stderr, "Failed to add text part to Gemini message.\n");
        dp_free_messages(messages, request_config.num_messages);
        dp_destroy_context(context);
        curl_global_cleanup();
        return 1;
    }
    
    printf("Sending streaming request to Gemini model: %s\n", request_config.model);
    printf("Prompt: %s\n---\nStreaming Response (Gemini via SSE with Disaster Party):\n", messages[0].parts[0].text);
    fflush(stdout); // Ensure prompt is printed before any potential stream output or debug output

    dp_response_t response_status = {0};
    int result = dp_perform_streaming_completion(context, &request_config, gemini_stream_handler_dp, NULL, &response_status);

    // Ensure a newline after all streaming output (stdout) and before final status messages (stdout/stderr)
    printf("\n---\n"); 
    fflush(stdout);

    if (result == 0) {
        printf("Gemini Streaming completed. HTTP Status: %ld\n", response_status.http_status_code);
         if (response_status.finish_reason) {
            printf("Finish Reason: %s\n", response_status.finish_reason);
        }
        // error_message in response_status for streaming indicates setup/transport errors
        // or unhandled errors propagated from the stream end by the library.
        if (response_status.error_message) {
             fprintf(stderr, "Overall operation reported an error by the library: %s\n", response_status.error_message);
        }
    } else { // result != 0 means a significant setup/transport error before or during stream start
        fprintf(stderr, "Gemini Streaming request setup failed. HTTP Status: %ld\n", response_status.http_status_code);
        if (response_status.error_message) {
            fprintf(stderr, "Error from library: %s\n", response_status.error_message);
        }
    }
    fflush(stdout);
    fflush(stderr);

    dp_free_response_content(&response_status);
    dp_free_messages(messages, request_config.num_messages);
    dp_destroy_context(context);
    curl_global_cleanup();
    printf("Gemini streaming test (Disaster Party) finished.\n");
    
    // Determine overall test success. If the library reported an error, or HTTP status wasn't 200, consider it a failure.
    // The callback might also indicate failure by returning non-zero, but dp_perform_streaming_completion doesn't directly reflect that.
    return (result == 0 && response_status.error_message == NULL && response_status.http_status_code == 200) ? 0 : 1;
}

