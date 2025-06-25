#include "disasterparty.h" 
#include <curl/curl.h> 
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 
#include <stdint.h> 
#include <stdbool.h>

char* base64_encode(const unsigned char *data, size_t input_length) {
    const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t output_length = 4 * ((input_length + 2) / 3);
    char *encoded_data = malloc(output_length + 1);
    if (encoded_data == NULL) {
        return NULL;
    }

    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = base64_chars[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = base64_chars[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = base64_chars[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = base64_chars[(triple >> 0 * 6) & 0x3F];
    }

    for (size_t i = 0; i < (3 - input_length % 3) % 3; i++) {
        encoded_data[output_length - 1 - i] = '=';
    }
    encoded_data[output_length] = '\0';
    return encoded_data;
}

unsigned char* read_file_to_buffer(const char* filename, size_t* file_size) {
    FILE* f = fopen(filename, "rb");
    if (!f) { 
        perror("fopen"); 
        return NULL; 
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size < 0) { 
        fclose(f); 
        perror("ftell"); 
        return NULL; 
    }
    *file_size = (size_t)size;
    fseek(f, 0, SEEK_SET);

    unsigned char* buffer = malloc(*file_size);
    if (!buffer) { 
        fclose(f); 
        fprintf(stderr, "Failed to allocate buffer for file %s\n", filename); 
        return NULL; 
    }

    if (fread(buffer, 1, *file_size, f) != *file_size) { 
        fclose(f); 
        free(buffer); 
        fprintf(stderr, "Failed to read file %s\n", filename); 
        return NULL; 
    }

    fclose(f);
    return buffer;
}

int main(int argc, char *argv[]) {
    const char* api_key = getenv("GEMINI_API_KEY");
    if (!api_key) {
        printf("SKIP: GEMINI_API_KEY environment variable not set.\n");
        return 77;
    }

    const char* image_path = NULL;
    if (argc > 1) { 
        image_path = argv[1]; 
    } else { 
        image_path = getenv("GEMINI_TEST_IMAGE_PATH");
    }

    if (!image_path) {
        printf("SKIP: Image path not provided. Use %s [path] or set GEMINI_TEST_IMAGE_PATH\n", argv[0]);
        return 77;
    }

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed.\n");
        return EXIT_FAILURE;
    }

    const char* model_env = getenv("GEMINI_MODEL_VISION");
    const char* model_to_use = model_env ? model_env : "gemini-2.0-flash";

    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Using Gemini API Key: ***\n");
    if (argc > 1) printf("Using image path from argument: %s\n", image_path);
    else printf("Using image path from GEMINI_TEST_IMAGE_PATH: %s\n", image_path);

    dp_context_t* context = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context for Gemini (Multimodal).\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    printf("Disaster Party Context Initialized (Multimodal).\n");

    dp_request_config_t request_config = {0};
    request_config.model = model_to_use;
    request_config.temperature = 0.4;
    request_config.max_tokens = 2048;
    request_config.stream = false;

    dp_message_t messages[1];
    memset(messages, 0, sizeof(messages));
    request_config.messages = messages;
    request_config.num_messages = 1;

    messages[0].role = DP_ROLE_USER;

    if (!dp_message_add_text_part(&messages[0], "Describe this image.")) {
        fprintf(stderr, "Failed to add text part to Gemini multimodal message.\n");
        dp_destroy_context(context); 
        curl_global_cleanup(); 
        return EXIT_FAILURE;
    }

    size_t image_size;
    unsigned char* image_buffer = read_file_to_buffer(image_path, &image_size);
    if (!image_buffer) {
        fprintf(stderr, "FAIL: Could not read image file %s\n", image_path);
        dp_free_messages(messages, 1);
        dp_destroy_context(context);
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    
    char* base64_image_data = base64_encode(image_buffer, image_size);
    free(image_buffer);
    if (!base64_image_data) {
        fprintf(stderr, "FAIL: Base64 encoding failed for %s\n", image_path);
        dp_free_messages(messages, 1);
        dp_destroy_context(context);
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    
    const char* mime_type = (strstr(image_path, ".png") || strstr(image_path, ".PNG")) ? "image/png" : "image/jpeg";
    if (!dp_message_add_base64_image_part(&messages[0], mime_type, base64_image_data)) {
        fprintf(stderr, "Failed to add base64 image part to Gemini message.\n");
        free(base64_image_data);
        dp_free_messages(messages, 1);
        dp_destroy_context(context);
        curl_global_cleanup();
        return EXIT_FAILURE;
    }
    free(base64_image_data);

    printf("Sending multimodal request to Gemini model: %s\n", request_config.model);
    
    dp_response_t response = {0};
    int result = dp_perform_completion(context, &request_config, &response);

    if (result == 0 && response.num_parts > 0 && response.parts[0].type == DP_CONTENT_PART_TEXT) {
        printf("\n--- Gemini Multimodal Completion Response (HTTP %ld) ---\n", response.http_status_code);
        printf("%s\n", response.parts[0].text);
        if (response.finish_reason) printf("Finish Reason: %s\n", response.finish_reason);
        printf("---------------------------------------------------\n");
    } else {
        fprintf(stderr, "\n--- Gemini Multimodal Completion Failed (HTTP %ld) ---\n", response.http_status_code);
        if (response.error_message) { 
            fprintf(stderr, "Error: %s\n", response.error_message); 
        } else { 
            fprintf(stderr, "An unknown error occurred.\n"); 
        }
        printf("---------------------------------------------------\n");
    }

    bool success = (result == 0 && response.error_message == NULL && response.http_status_code == 200);
    int final_exit_code = success ? EXIT_SUCCESS : EXIT_FAILURE;
    
    dp_free_response_content(&response);
    dp_free_messages(messages, 1);
    dp_destroy_context(context);
    curl_global_cleanup();
    printf("Gemini multimodal test (Disaster Party) finished.\n");
    
    return final_exit_code;
}
