#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <disasterparty.h>

#define API_KEY_ENV "GEMINI_API_KEY"
#define TEST_FILE_CONTENT "This is a test text file content for Gemini multimodal attachment."
#define TEST_FILE_MIME_TYPE "text/plain"
#define TEST_FILE_BASE64 "VGhpcyBpcyBhIHRlc3QgdGV4dCBmaWxlIGNvbnRlbnQgZm9yIEdlbWluaSBtdWx0aW1vZGFsIGF0dGFjaG1lbnQu" // Base64 of TEST_FILE_CONTENT

static int test_count = 0;
static int test_failures = 0;

#define RUN_TEST(test_func) \
    do { \
        test_count++; \
        printf("Running %s...\n", #test_func); \
        if (test_func() != 0) { \
            test_failures++; \
            printf("FAIL: %s\n", #test_func); \
        } else { \
            printf("PASS: %s\n", #test_func); \
        } \
    } while (0)

static int test_gemini_text_file_attachment() {
    dp_context_t* context = NULL;
    dp_request_config_t config = {0};
    dp_response_t response = {0};
    dp_message_t message = {0};
    int ret = -1;

    const char* api_key = getenv(API_KEY_ENV);
    if (!api_key) {
        fprintf(stderr, "Skipping test_gemini_text_file_attachment: %s not set.\n", API_KEY_ENV);
        return 0; // Skip test
    }

    context = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, api_key, NULL);
    assert(context != NULL);

    message.role = DP_ROLE_USER;
    assert(dp_message_add_text_part(&message, "Analyze the attached text file and summarize its content."));
    assert(dp_message_add_base64_image_part(&message, TEST_FILE_MIME_TYPE, TEST_FILE_BASE64));

    config.model = "gemini-pro"; // Or a suitable multimodal model
    config.messages = &message;
    config.num_messages = 1;
    config.max_tokens = 100;
    config.temperature = 0.0;

    ret = dp_perform_completion(context, &config, &response);

    printf("HTTP Status: %ld\n", response.http_status_code);
    if (response.error_message) {
        printf("Error: %s\n", response.error_message);
    }
    if (response.finish_reason) {
        printf("Finish Reason: %s\n", response.finish_reason);
    }

    assert(ret == 0);
    assert(response.http_status_code >= 200 && response.http_status_code < 300);
    assert(response.parts != NULL);
    assert(response.num_parts > 0);
    assert(response.parts[0].text != NULL);
    printf("Response: %s\n", response.parts[0].text);
    assert(strstr(response.parts[0].text, "test text file content") != NULL || strstr(response.parts[0].text, "summarize") != NULL);

    dp_free_response_content(&response);
    dp_free_messages(&message, 1);
    dp_destroy_context(context);

    return 0;
}

int main() {
    printf("Starting Gemini File Attachment Tests...\n");
    RUN_TEST(test_gemini_text_file_attachment);
    printf("Gemini File Attachment Tests finished.\n");

    if (test_failures > 0) {
        printf("Total tests run: %d, Failures: %d\n", test_count, test_failures);
        return 1;
    } else {
        printf("All %d tests passed.\n", test_count);
        return 0;
    }
}