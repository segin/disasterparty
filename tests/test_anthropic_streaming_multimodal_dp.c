#include "disasterparty.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

// Re-using helper functions from other tests
char* base64_encode(const unsigned char *data, size_t input_length);
unsigned char* read_file_to_buffer(const char* filename, size_t* file_size);

int stream_handler(const char* token, void* user_data, bool is_final, const char* error_msg) {
    if (error_msg) {
        fprintf(stderr, "\nStream Error: %s\n", error_msg);
        return 1;
    }
    if (token) {
        printf("%s", token);
        fflush(stdout);
    }
    if (is_final && !error_msg) {
         printf("\n[STREAM END - Anthropic Multimodal]\n");
    }
    return 0;
}

int main(int argc, char* argv[]) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    const char* api_key = getenv("ANTHROPIC_API_KEY");
    if (!api_key) {
        fprintf(stderr, "Error: ANTHROPIC_API_KEY environment variable not set.\n");
        return 1;
    }

    const char* image_path = NULL;
    if (argc > 1) { image_path = argv[1]; }
    else { image_path = getenv("ANTHROPIC_TEST_IMAGE_PATH");}
    if (!image_path) {
        fprintf(stderr, "FAIL: Image path required. Usage: %s [path] or set ANTHROPIC_TEST_IMAGE_PATH\n", argv[0]);
        return 1;
    }


    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Testing Anthropic Streaming Multimodal:\n");

    dp_context_t* context = dp_init_context(DP_PROVIDER_ANTHROPIC, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize context for Anthropic.\n");
        return 1;
    }

    dp_request_config_t request_config = {0};
    request_config.model = "claude-3-haiku-20240307";
    request_config.temperature = 0.5;
    request_config.max_tokens = 512;
    request_config.stream = true;

    dp_message_t messages[1];
    request_config.messages = messages;
    request_config.num_messages = 1;

    messages[0].role = DP_ROLE_USER;
    messages[0].num_parts = 0;
    messages[0].parts = NULL;

    if (!dp_message_add_text_part(&messages[0], "What is in this image? Describe it in detail, streaming your response.")) {
        return 1;
    }

    size_t image_size;
    unsigned char* image_buffer = read_file_to_buffer(image_path, &image_size);
    if (!image_buffer) return 1;

    char* base64_image_data = base64_encode(image_buffer, image_size);
    free(image_buffer);
    if (!base64_image_data) { fprintf(stderr, "Base64 encoding failed.\n"); return 1; }

    const char* mime_type = (strstr(image_path, ".png") || strstr(image_path, ".PNG")) ? "image/png" : "image/jpeg";
    if (!dp_message_add_base64_image_part(&messages[0], mime_type, base64_image_data)) {
        free(base64_image_data);
        return 1;
    }
    free(base64_image_data);

    printf("Sending streaming multimodal request to model: %s\n---\n", request_config.model);

    dp_response_t response_status = {0};
    int result = dp_perform_streaming_completion(context, &request_config, stream_handler, NULL, &response_status);

    printf("\n---\n");
    if (result == 0) {
        printf("Anthropic streaming multimodal test seems successful. HTTP: %ld\n", response_status.http_status_code);
    } else {
        fprintf(stderr, "Anthropic streaming multimodal test failed. HTTP: %ld, Error: %s\n", response_status.http_status_code, response_status.error_message ? response_status.error_message : "N/A");
    }

    // Determine exit code based on success conditions
    bool success = (result == 0 && response_status.error_message == NULL && response_status.http_status_code == 200);
    int final_exit_code = success ? 0 : 1;

    dp_free_response_content(&response_status);
    dp_free_messages(messages, 1);
    dp_destroy_context(context);
    curl_global_cleanup();

    return final_exit_code;
}

char* base64_encode(const unsigned char *data, size_t input_length) { const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; size_t output_length = 4 * ((input_length + 2) / 3); char *encoded_data = malloc(output_length + 1); if (encoded_data == NULL) return NULL; for (size_t i = 0, j = 0; i < input_length;) { uint32_t octet_a = i < input_length ? data[i++] : 0; uint32_t octet_b = i < input_length ? data[i++] : 0; uint32_t octet_c = i < input_length ? data[i++] : 0; uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c; encoded_data[j++] = base64_chars[(triple >> 3 * 6) & 0x3F]; encoded_data[j++] = base64_chars[(triple >> 2 * 6) & 0x3F]; encoded_data[j++] = base64_chars[(triple >> 1 * 6) & 0x3F]; encoded_data[j++] = base64_chars[(triple >> 0 * 6) & 0x3F]; } for (size_t i = 0; i < (3 - input_length % 3) % 3; i++) { encoded_data[output_length - 1 - i] = '='; } encoded_data[output_length] = '\0'; return encoded_data; }
unsigned char* read_file_to_buffer(const char* filename, size_t* file_size) { FILE* f = fopen(filename, "rb"); if (!f) { perror("fopen"); return NULL; } fseek(f, 0, SEEK_END); long size = ftell(f); if (size < 0) { fclose(f); perror("ftell"); return NULL; } *file_size = (size_t)size; fseek(f, 0, SEEK_SET); unsigned char* buffer = malloc(*file_size); if (!buffer) { fclose(f); fprintf(stderr, "Failed to allocate buffer for file %s\n", filename); return NULL; } if (fread(buffer, 1, *file_size, f) != *file_size) { fclose(f); free(buffer); fprintf(stderr, "Failed to read file %s\n", filename); return NULL; } fclose(f); return buffer; }
