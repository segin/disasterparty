#define _GNU_SOURCE
#include "disasterparty.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

int main() {
    const char* api_key = getenv("GEMINI_API_KEY");
    // Gemini usually uses the standard Google API URL, but we allow override
    const char* base_url = getenv("GEMINI_API_BASE_URL"); 
    const char* model_env = getenv("GEMINI_MODEL");

    if (!api_key) {
        printf("SKIP: GEMINI_API_KEY not set.\n");
        return 77; // Automake's standard skip code
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed.\n");
        return EXIT_FAILURE;
    }

    const char* model_to_use = model_env ? model_env : "gemini-1.5-pro-latest";
    // Default Gemini base URL if not set
    const char* url_to_use = base_url ? base_url : "https://generativelanguage.googleapis.com/v1beta";

    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Using Gemini API Key: ***\n");
    printf("Using Gemini Base URL: %s\n", url_to_use);

    dp_context_t* context = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, api_key, url_to_use);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context.\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }

    // --- Step 1: Define Tool ---
    dp_tool_definition_t tools[1];
    tools[0].type = DP_TOOL_TYPE_FUNCTION;
    tools[0].function.name = "get_weather";
    tools[0].function.description = "Get the current weather in a given location";
    tools[0].function.parameters_json_schema = "{"
        "\"type\": \"object\","
        "\"properties\": {"
            "\"location\": {"
                "\"type\": \"string\","
                "\"description\": \"The city and state, e.g. San Francisco, CA\""
            "},"
            "\"unit\": {"
                "\"type\": \"string\","
                "\"enum\": [\"celsius\", \"fahrenheit\"]"
            "}"
        "},"
        "\"required\": [\"location\"]"
    "}";

    // --- Step 2: Initial Request ---
    dp_message_t messages[3]; // Allocate enough for the conversation loop
    memset(messages, 0, sizeof(messages));

    // User Message
    messages[0].role = DP_ROLE_USER;
    if (!dp_message_add_text_part(&messages[0], "What is the weather in Paris?")) {
        fprintf(stderr, "Failed to add text part.\n");
        return EXIT_FAILURE;
    }

    dp_request_config_t request_config = {0};
    request_config.model = model_to_use;
    request_config.messages = messages;
    request_config.num_messages = 1;
    request_config.tools = tools;
    request_config.num_tools = 1;
    request_config.tool_choice.type = DP_TOOL_CHOICE_AUTO;

    printf("\n--- Step 1: Sending Tool Call Request ---\n");
    dp_response_t response1 = {0};
    int result = dp_perform_completion(context, &request_config, &response1);

    char* tool_call_id = NULL;
    char* function_name = NULL;
    char* args_json = NULL;

    bool step1_success = false;
    if (result == 0 && response1.http_status_code == 200) {
        for (size_t i = 0; i < response1.num_parts; ++i) {
            if (response1.parts[i].type == DP_CONTENT_PART_TOOL_CALL) {
                printf("Received Tool Call:\n");
                printf("  ID: %s\n", response1.parts[i].tool_call.id);
                printf("  Function: %s\n", response1.parts[i].tool_call.function_name);
                printf("  Arguments: %s\n", response1.parts[i].tool_call.arguments_json);
                
                if (strcmp(response1.parts[i].tool_call.function_name, "get_weather") == 0) {
                    step1_success = true;
                    // Save for next step
                    tool_call_id = strdup(response1.parts[i].tool_call.id);
                    function_name = strdup(response1.parts[i].tool_call.function_name);
                    args_json = strdup(response1.parts[i].tool_call.arguments_json);
                }
            }
        }
    } else {
        fprintf(stderr, "Request failed. HTTP: %ld, Error: %s\n", response1.http_status_code, response1.error_message);
    }

    if (!step1_success) {
        fprintf(stderr, "Step 1 Failed: Did not receive expected tool call.\n");
        if (tool_call_id) free(tool_call_id);
        if (function_name) free(function_name);
        if (args_json) free(args_json);
        dp_free_response_content(&response1);
        dp_free_messages(messages, 1);
        dp_destroy_context(context);
        curl_global_cleanup();
        return EXIT_FAILURE;
    }

    // --- Step 2: Submit Tool Result ---
    printf("\n--- Step 2: Sending Tool Result ---\n");

    // 1. Assistant Message (The tool call itself)
    messages[1].role = DP_ROLE_ASSISTANT;
    dp_message_add_tool_call_part(&messages[1], tool_call_id, function_name, args_json);
    
    // 2. Tool Message (The result)
    messages[2].role = DP_ROLE_TOOL;
    const char* mock_response = "{\"temperature\": \"15\", \"unit\": \"celsius\", \"description\": \"Partly cloudy\"}";
    
    // Note: Gemini uses the function name (or id provided in the call) to link results.
    // In our implementation, we pass the tool_call_id.
    dp_message_add_tool_result_part(&messages[2], tool_call_id, mock_response, false);

    // Update config
    request_config.num_messages = 3;
    
    dp_response_t response2 = {0};
    result = dp_perform_completion(context, &request_config, &response2);

    bool step2_success = false;
    if (result == 0 && response2.http_status_code == 200) {
        for (size_t i = 0; i < response2.num_parts; ++i) {
            if (response2.parts[i].type == DP_CONTENT_PART_TEXT) {
                printf("Received Final Response: %s\n", response2.parts[i].text);
                // Verify the model used the tool output
                if (strstr(response2.parts[i].text, "15") || strstr(response2.parts[i].text, "cloudy") || strstr(response2.parts[i].text, "Paris")) {
                    step2_success = true;
                }
            }
        }
    } else {
        fprintf(stderr, "Request failed. HTTP: %ld, Error: %s\n", response2.http_status_code, response2.error_message);
    }

    // Cleanup
    if (tool_call_id) free(tool_call_id);
    if (function_name) free(function_name);
    if (args_json) free(args_json);
    dp_free_response_content(&response1);
    dp_free_response_content(&response2);
    dp_free_messages(messages, 3);
    dp_destroy_context(context);
    curl_global_cleanup();

    if (step2_success) {
        printf("\nSUCCESS: Tool calling flow completed successfully.\n");
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "\nFAILURE: Step 2 failed or did not incorporate tool result.\n");
        return EXIT_FAILURE;
    }
}