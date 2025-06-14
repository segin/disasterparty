#include "disasterparty.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Helper function to run a specific error test
bool run_test(const char* test_name, dp_provider_type_t provider, const char* api_key, const char* model, long expected_http_status) {
    printf("\n--- Running Test: %s ---\n", test_name);
    fflush(stdout);

    dp_context_t* context = dp_init_context(provider, api_key, NULL);
    if (!context) {
        fprintf(stderr, "FAIL: Context initialization failed for test: %s\n", test_name);
        return false;
    }

    dp_request_config_t request_config = {0};
    request_config.model = model;
    request_config.temperature = 0.5;
    request_config.max_tokens = 5;

    dp_message_t messages[1];
    memset(messages, 0, sizeof(messages));
    request_config.messages = messages;
    request_config.num_messages = 1;
    messages[0].role = DP_ROLE_USER;
    dp_message_add_text_part(&messages[0], "hello");

    dp_response_t response = {0};
    dp_perform_completion(context, &request_config, &response);

    bool success = (response.http_status_code == expected_http_status && response.error_message != NULL);

    if (success) {
        printf("PASS: Successfully caught expected HTTP %ld.\n", expected_http_status);
        printf("      API Error Message: %.100s...\n", response.error_message);
    } else {
        fprintf(stderr, "FAIL: Expected HTTP %ld but got %ld. Library Error: \"%s\"\n",
                expected_http_status,
                response.http_status_code,
                response.error_message ? response.error_message : "None");
    }

    dp_free_response_content(&response);
    dp_free_messages(messages, 1);
    dp_destroy_context(context);

    return success;
}

int main() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed.\n");
        return EXIT_FAILURE;
    }

    const char* openai_key = getenv("OPENAI_API_KEY");
    const char* gemini_key = getenv("GEMINI_API_KEY");
    const char* anthropic_key = getenv("ANTHROPIC_API_KEY");
    const char* fake_key = "invalid-api-key-for-testing";
    const char* fake_model = "model-does-not-exist";

    if (!openai_key && !gemini_key && !anthropic_key) {
        printf("SKIP: No API keys (OPENAI_API_KEY, GEMINI_API_KEY, ANTHROPIC_API_KEY) are set. Skipping all error handling tests.\n");
        curl_global_cleanup();
        return 77; // Automake skip code
    }

    bool all_tests_passed = true;
    int tests_run = 0;

    // Test Bad API Key
    if (openai_key) {
        tests_run++;
        if (!run_test("OpenAI Bad API Key", DP_PROVIDER_OPENAI_COMPATIBLE, fake_key, "gpt-4o", 401)) all_tests_passed = false;
    }
    if (gemini_key) {
        tests_run++;
        if (!run_test("Gemini Bad API Key", DP_PROVIDER_GOOGLE_GEMINI, fake_key, "gemini-2.0-flash", 400)) all_tests_passed = false;
    }
    if (anthropic_key) {
        tests_run++;
        if (!run_test("Anthropic Bad API Key", DP_PROVIDER_ANTHROPIC, fake_key, "claude-3-haiku-20240307", 401)) all_tests_passed = false;
    }

    // Test Bad Model Name
    if (openai_key) {
        tests_run++;
        if (!run_test("OpenAI Bad Model Name", DP_PROVIDER_OPENAI_COMPATIBLE, openai_key, fake_model, 404)) all_tests_passed = false;
    }
    if (gemini_key) {
        tests_run++;
        if (!run_test("Gemini Bad Model Name", DP_PROVIDER_GOOGLE_GEMINI, gemini_key, fake_model, 404)) all_tests_passed = false;
    }
    if (anthropic_key) {
        tests_run++;
        if (!run_test("Anthropic Bad Model Name", DP_PROVIDER_ANTHROPIC, anthropic_key, fake_model, 404)) all_tests_passed = false;
    }

    curl_global_cleanup();

    if (all_tests_passed) {
        printf("\nAll %d error handling tests passed!\n", tests_run);
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "\nSome of the %d error handling tests failed.\n", tests_run);
        return EXIT_FAILURE;
    }
}
