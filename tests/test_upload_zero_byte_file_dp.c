#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    const char* mock_server_url = getenv("DP_MOCK_SERVER");
    if (!mock_server_url) {
        printf("SKIP: DP_MOCK_SERVER environment variable not set.\n");
        return 77;
    }

    printf("Testing dp_upload_file with a zero-byte file...\n");

    // Create a zero-byte file
    const char* zero_byte_file_path = "./zero_byte_test_file.txt";
    FILE* fp = fopen(zero_byte_file_path, "w");
    if (fp == NULL) {
        fprintf(stderr, "Failed to create zero-byte file.\n");
        return EXIT_FAILURE;
    }
    fclose(fp);

    dp_context_t* context = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, "ZERO_BYTE_FILE", mock_server_url);
    if (!context) {
        fprintf(stderr, "Failed to initialize context for mock server.\n");
        remove(zero_byte_file_path);
        return EXIT_FAILURE;
    }

    dp_file_t* uploaded_file = NULL;
    int ret = dp_upload_file(context, zero_byte_file_path, "text/plain", &uploaded_file);

    if (ret != 0) {
        printf("SUCCESS: dp_upload_file correctly failed for a zero-byte file (return code: %d).\n", ret);
        if (uploaded_file && uploaded_file->http_status_code == 400) {
            printf("SUCCESS: Received expected HTTP 400 (Bad Request) status code.\n");
        }
        dp_free_file(uploaded_file);
        dp_destroy_context(context);
        remove(zero_byte_file_path);
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "FAILURE: dp_upload_file succeeded unexpectedly for a zero-byte file.\n");
        dp_free_file(uploaded_file);
        dp_destroy_context(context);
        remove(zero_byte_file_path);
        return EXIT_FAILURE;
    }
}