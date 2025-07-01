#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <disasterparty.h>

#define API_KEY_ENV "GEMINI_API_KEY"
#define TEST_FILE_CONTENT "This is a test text file content for Gemini multimodal attachment."
#define TEST_FILE_MIME_TYPE "text/plain"
#define TEST_FILE_NAME "test_gemini_attachment.txt"

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

static int test_gemini_file_attachment() {
    dp_context_t* context = NULL;
    dp_request_config_t config = {0};
    dp_response_t response = {0};
    dp_message_t message = {0};
    dp_file_t* uploaded_file = NULL;
    int ret = -1;

    const char* api_key = getenv(API_KEY_ENV);
    if (!api_key) {
        fprintf(stderr, "Skipping test_gemini_file_attachment: %s not set.\n", API_KEY_ENV);
        return 0; // Skip test
    }

    printf("Attempting to initialize context...\n");
    context = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, api_key, NULL);
    assert(context != NULL);
    printf("Context initialized. Attempting to create dummy file...\n");

    // Create a dummy file for upload
    FILE* fp = fopen(TEST_FILE_NAME, "w");
    if (!fp) {
        fprintf(stderr, "Failed to create dummy file %s\n", TEST_FILE_NAME);
        return 1;
    }
    fputs(TEST_FILE_CONTENT, fp);
    fclose(fp);
    printf("Dummy file created. Attempting to upload file...\n");

    // Upload the file
    printf("Calling dp_upload_file...\n");
    ret = dp_upload_file(context, TEST_FILE_NAME, TEST_FILE_MIME_TYPE, &uploaded_file);
    printf("dp_upload_file returned: %d\n", ret);
    if (ret != 0 || uploaded_file == NULL) {
        fprintf(stderr, "File upload failed: %d\n", ret);
        dp_destroy_context(context);
        remove(TEST_FILE_NAME);
        return 1;
    }
    printf("File uploaded successfully. URI: %s\n", uploaded_file->uri);

    // Removed completion part for isolation

    printf("Freeing resources...\n");
    dp_free_file(uploaded_file);
    dp_destroy_context(context);
    remove(TEST_FILE_NAME);
    printf("Resources freed. Test finished.\n");

    return 0;
}

int main() {
    printf("Starting Gemini File Attachment Tests...\n");
    RUN_TEST(test_gemini_file_attachment);
    printf("Gemini File Attachment Tests finished.\n");

    if (test_failures > 0) {
        printf("Total tests run: %d, Failures: %d\n", test_count, test_failures);
        return 1;
    } else {
        printf("All %d tests passed.\n", test_count);
        return 0;
    }
}
