#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main() {
    dp_file_t* uploaded_file = NULL;
    int ret;
    int final_ret = EXIT_SUCCESS;

    // --- Create a dummy file to attempt to upload ---
    char temp_filename_template[] = "test_unsupported_upload_XXXXXX";
    const char* tmpdir = getenv("TMPDIR");
    if (tmpdir == NULL) {
        tmpdir = ".";
    }
    char* temp_filename = malloc(strlen(tmpdir) + strlen(temp_filename_template) + 2);
    sprintf(temp_filename, "%s/%s", tmpdir, temp_filename_template);

    int fd = mkstemp(temp_filename);
    if (fd == -1) {
        perror("mkstemp");
        free(temp_filename);
        return EXIT_FAILURE;
    }
    if (write(fd, "This file should not be uploaded.", 33) == -1) {
        perror("write");
        close(fd);
        unlink(temp_filename);
        free(temp_filename);
        return EXIT_FAILURE;
    }
    close(fd);

    // --- Test with OpenAI Provider (should fail - not supported) ---
    printf("Testing unsupported file upload for OpenAI...\n");
    dp_context_t* openai_context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, "dummy_key", NULL);
    if (!openai_context) {
        fprintf(stderr, "Failed to initialize context for OpenAI.\n");
        unlink(temp_filename);
        free(temp_filename);
        return EXIT_FAILURE;
    }

    ret = dp_upload_file(openai_context, temp_filename, "text/plain", &uploaded_file);
    if (ret == 0) {
        fprintf(stderr, "FAIL: dp_upload_file succeeded for OpenAI when it should have failed.\n");
        final_ret = EXIT_FAILURE;
    } else {
        printf("PASS: dp_upload_file correctly failed for the OpenAI provider.\n");
    }
    dp_free_file(uploaded_file); // Should be a no-op as uploaded_file should be NULL
    uploaded_file = NULL;
    dp_destroy_context(openai_context);

    // --- Test with Anthropic Provider (should fail - not supported) ---
    printf("Testing unsupported file upload for Anthropic...\n");
    dp_context_t* anthropic_context = dp_init_context(DP_PROVIDER_ANTHROPIC, "dummy_key", NULL);
    if (!anthropic_context) {
        fprintf(stderr, "Failed to initialize context for Anthropic.\n");
        unlink(temp_filename);
        free(temp_filename);
        return EXIT_FAILURE;
    }

    ret = dp_upload_file(anthropic_context, temp_filename, "text/plain", &uploaded_file);
    if (ret == 0) {
        fprintf(stderr, "FAIL: dp_upload_file succeeded for Anthropic when it should have failed.\n");
        final_ret = EXIT_FAILURE;
    } else {
        printf("PASS: dp_upload_file correctly failed for the Anthropic provider.\n");
    }
    dp_free_file(uploaded_file); // Should be a no-op
    dp_destroy_context(anthropic_context);

    // --- Cleanup ---
    unlink(temp_filename);
    free(temp_filename);

    return final_ret;
}