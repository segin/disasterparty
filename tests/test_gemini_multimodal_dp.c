#include "disasterparty.h" 
#include <stdio.h>
#include <stdlib.h> 
#include <string.h> 

char* base64_encode(const unsigned char *data, size_t input_length) {
    const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t output_length = 4 * ((input_length + 2) / 3);
    char *encoded_data = malloc(output_length + 1);
    if (encoded_data == NULL) return NULL;
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
    if (!f) { perror("fopen"); return NULL; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size < 0) { fclose(f); perror("ftell"); return NULL; }
    *file_size = (size_t)size;
    fseek(f, 0, SEEK_SET);
    unsigned char* buffer = malloc(*file_size);
    if (!buffer) { fclose(f); fprintf(stderr, "Failed to allocate buffer for file %s\n", filename); return NULL; }
    if (fread(buffer, 1, *file_size, f) != *file_size) { fclose(f); free(buffer); fprintf(stderr, "Failed to read file %s\n", filename); return NULL; }
    fclose(f);
    return buffer;
}

int main(int argc, char *argv[]) {
    CURLcode global_init_res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (global_init_res != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed: %s\n", curl_easy_strerror(global_init_res));
        return 1;
    }

    const char* api_key = getenv("GEMINI_API_KEY");
    if (!api_key) {
        fprintf(stderr, "Error: GEMINI_API_KEY environment variable not set.\n");
        curl_global_cleanup();
        return 1;
    }
    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Using Gemini API Key: ***\n");

    const char* image_path = NULL;
    if (argc > 1) { image_path = argv[1]; printf("Using image path from argument: %s\n", image_path); } 
    else { image_path = getenv("GEMINI_TEST_IMAGE_PATH"); if (image_path) printf("Using image path from GEMINI_TEST_IMAGE_PATH: %s\n", image_path);
           else { fprintf(stderr, "Usage: %s [path_to_image.jpg/png] or set GEMINI_TEST_IMAGE_PATH\n", argv[0]);}}

    dp_context_t* context = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context for Gemini (Multimodal).\n");
        curl_global_cleanup();
        return 1;
    }
    printf("Disaster Party Context Initialized (Multimodal).\n");

    dp_request_config_t request_config = {0};
    request_config.model = "gemini-1.5-flash-latest"; 
    request_config.temperature = 0.4;
    request_config.max_tokens = 256;
    request_config.stream = false;

    dp_message_t messages[1];
    request_config.messages = messages;
    request_config.num_messages = 1;

    messages[0].role = DP_ROLE_USER;
    messages[0].num_parts = 0;
    messages[0].parts = NULL;

    if (!dp_message_add_text_part(&messages[0], "Describe this image.")) {
        fprintf(stderr, "Failed to add text part to Gemini multimodal message.\n");
        dp_free_messages(messages, request_config.num_messages);
        dp_destroy_context(context); curl_global_cleanup(); return 1;
    }

    char* base64_image_data = NULL;
    unsigned char* image_buffer = NULL;

    if (image_path) {
        size_t image_size;
        image_buffer = read_file_to_buffer(image_path, &image_size);
        if (image_buffer) {
            base64_image_data = base64_encode(image_buffer, image_size);
            if (base64_image_data) {
                const char* mime_type = (strstr(image_path, ".png") || strstr(image_path, ".PNG")) ? "image/png" : "image/jpeg";
                if (!dp_message_add_base64_image_part(&messages[0], mime_type, base64_image_data)) {
                    fprintf(stderr, "Failed to add base64 image part to Gemini message.\n");
                } else { printf("Added base64 image part (MIME: %s)\n", mime_type); }
            } else { fprintf(stderr, "Failed to base64 encode image data from %s.\n", image_path); }
            free(image_buffer); 
        } else { fprintf(stderr, "Failed to read image file %s.\n", image_path); }
    } else { printf("No image path provided. Sending text-only request to multimodal endpoint.\n"); }

    printf("Sending multimodal request to Gemini model: %s\n", request_config.model);
    if (messages[0].num_parts > 0 && messages[0].parts[0].text) { 
      printf("Text prompt: %s\n", messages[0].parts[0].text);
    }


    dp_response_t response = {0};
    int result = dp_perform_completion(context, &request_config, &response);

    if (result == 0 && response.num_parts > 0 && response.parts[0].type == DP_CONTENT_PART_TEXT) {
        printf("\n--- Gemini Multimodal Completion Response (HTTP %ld) ---\n", response.http_status_code);
        printf("%s\n", response.parts[0].text);
        if (response.finish_reason) printf("Finish Reason: %s\n", response.finish_reason);
        printf("---------------------------------------------------\n");
    } else {
        fprintf(stderr, "\n--- Gemini Multimodal Completion Failed (HTTP %ld) ---\n", response.http_status_code);
        if (response.error_message) { fprintf(stderr, "Error: %s\n", response.error_message); } 
        else { fprintf(stderr, "An unknown error occurred.\n"); }
        fprintf(stderr, "---------------------------------------------------\n");
    }

    free(base64_image_data);      
    dp_free_response_content(&response);
    dp_free_messages(messages, request_config.num_messages);
    dp_destroy_context(context);
    curl_global_cleanup();
    printf("Gemini multimodal test (Disaster Party) finished.\n");
    return result == 0 ? 0 : 1;
}

