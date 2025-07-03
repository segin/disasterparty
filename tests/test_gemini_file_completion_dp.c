#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

int main() {
    const char* api_key = getenv("GEMINI_API_KEY");
    if (!api_key) {
        fprintf(stderr, "GEMINI_API_KEY not set, skipping test.\n");
        return 77; // Skip test
    }

    // 1. Create a temporary file
    char temp_filename[] = "/tmp/dp_test_file_XXXXXX";
    int fd = mkstemp(temp_filename);
    if (fd == -1) {
        perror("mkstemp");
        return 1;
    }
    const char* file_content = "This is a test file for disasterparty library. The capital of France is Paris.";
    if (write(fd, file_content, strlen(file_content)) == -1) {
        perror("write");
        close(fd);
        unlink(temp_filename);
        return 1;
    }
    close(fd);

    dp_context_t* dp_ctx = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, api_key, NULL);
    if (!dp_ctx) {
        fprintf(stderr, "Failed to initialize context.\n");
        unlink(temp_filename);
        return 1;
    }

    // 2. Upload the file
    dp_file_t* uploaded_file = NULL;
    if (dp_upload_file(dp_ctx, temp_filename, "text/plain", &uploaded_file) != 0) {
        fprintf(stderr, "File upload failed.\n");
        dp_free_file(uploaded_file);
        dp_destroy_context(dp_ctx);
        unlink(temp_filename);
        return 1;
    }

    assert(uploaded_file != NULL);
    assert(uploaded_file->uri != NULL);

    // 3. Create the message with the file reference
    dp_message_t messages[1] = {0};
    messages[0].role = DP_ROLE_USER;
    assert(dp_message_add_text_part(&messages[0], "What is the content of the provided file?"));
    assert(dp_message_add_file_reference_part(&messages[0], uploaded_file->uri));


    dp_request_config_t request_config = {
        .model = "gemini-2.5-flash",
        .messages = messages,
        .num_messages = 1,
    };

    dp_response_t response = {0};
    int result = dp_perform_completion(dp_ctx, &request_config, &response);

    // 5. Cleanup
    dp_free_file(uploaded_file);
    dp_free_messages(messages, 1);
    dp_destroy_context(dp_ctx);
    unlink(temp_filename);

    if (result != 0) {
        fprintf(stderr, "dp_perform_completion failed: %s\n", response.error_message);
        dp_free_response_content(&response);
        return 1;
    }

    assert(response.num_parts > 0);
    assert(response.parts[0].text != NULL);

    printf("Model response: %s\n", response.parts[0].text);

    // Check if the response contains keywords from the file
    assert(strstr(response.parts[0].text, "test file") != NULL);
    assert(strstr(response.parts[0].text, "disasterparty") != NULL);
    assert(strstr(response.parts[0].text, "Paris") != NULL);


    dp_free_response_content(&response);
    printf("Gemini file completion test passed!\n");
    return 0;
}

