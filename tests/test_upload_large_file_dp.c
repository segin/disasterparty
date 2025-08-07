#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LARGE_FILE_SIZE (1024 * 1024 * 101) // 101 MB

int main() {
    const char* mock_server_url = getenv("DP_MOCK_SERVER");
    if (!mock_server_url) {
        printf("SKIP: DP_MOCK_SERVER environment variable not set.\n");
        return 77;
    }

    printf("Testing dp_upload_file with a very large file (>100MB)...\n");

    const char* large_file_path = "./large_test_file.bin";
    FILE* fp = fopen(large_file_path, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Failed to create large test file.\n");
        return EXIT_FAILURE;
    }

    // Write synthetic data to the file
    char buffer[4096];
    memset(buffer, 'A', sizeof(buffer));
    size_t bytes_written = 0;
    while (bytes_written < LARGE_FILE_SIZE) {
        size_t to_write = sizeof(buffer);
        if (bytes_written + to_write > LARGE_FILE_SIZE) {
            to_write = LARGE_FILE_SIZE - bytes_written;
        }
        if (fwrite(buffer, 1, to_write, fp) != to_write) {
            fprintf(stderr, "Error writing to large test file.\n");
            fclose(fp);
            remove(large_file_path);
            return EXIT_FAILURE;
        }
        bytes_written += to_write;
    }
    fclose(fp);

    dp_context_t* context = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, "LARGE_FILE_UPLOAD", mock_server_url);
    if (!context) {
        fprintf(stderr, "Failed to initialize context for mock server.\n");
        remove(large_file_path);
        return EXIT_FAILURE;
    }

    dp_file_t* uploaded_file = NULL;
    int ret = dp_upload_file(context, large_file_path, "application/octet-stream", &uploaded_file);

    if (ret != 0) {
        printf("SUCCESS: dp_upload_file correctly failed for a large file (return code: %d).\n", ret);
        if (uploaded_file && uploaded_file->http_status_code == 413) {
            printf("SUCCESS: Received expected HTTP 413 (Payload Too Large) status code.\n");
        }
        dp_free_file(uploaded_file);
        dp_destroy_context(context);
        remove(large_file_path);
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "FAILURE: dp_upload_file succeeded unexpectedly for a large file.\n");
        dp_free_file(uploaded_file);
        dp_destroy_context(context);
        remove(large_file_path);
        return EXIT_FAILURE;
    }
}