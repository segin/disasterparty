#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

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


int main(int argc, char* argv[]) {
    printf("Running Inline Multimodal Context Test...\n");

    const char* image_path_1 = getenv("TEST_IMAGE_PATH_1");
    const char* image_path_2 = getenv("TEST_IMAGE_PATH_2");

    if (!image_path_1 || !image_path_2) {
        printf("SKIP: Set TEST_IMAGE_PATH_1 and TEST_IMAGE_PATH_2 to run this test.\n");
        return 77;
    }

    dp_message_t message = {0};
    message.role = DP_ROLE_USER;

    // Build the interleaved message: Text -> Image 1 -> Text -> Image 2
    dp_message_add_text_part(&message, "What is in the first image?");

    size_t img1_size;
    unsigned char* img1_buf = read_file_to_buffer(image_path_1, &img1_size);
    if (!img1_buf) return EXIT_FAILURE;
    char* b64_img1 = base64_encode(img1_buf, img1_size);
    free(img1_buf);
    dp_message_add_base64_image_part(&message, "image/jpeg", b64_img1);
    free(b64_img1);

    dp_message_add_text_part(&message, "And how is it different from this second image?");

    size_t img2_size;
    unsigned char* img2_buf = read_file_to_buffer(image_path_2, &img2_size);
    if (!img2_buf) return EXIT_FAILURE;
    char* b64_img2 = base64_encode(img2_buf, img2_size);
    free(img2_buf);
    dp_message_add_base64_image_part(&message, "image/jpeg", b64_img2);
    free(b64_img2);

    printf("Successfully built interleaved message with %zu parts.\n", message.num_parts);

    // This test doesn't call an API, it just verifies the message structure can be built.
    assert(message.num_parts == 4);
    assert(message.parts[0].type == DP_CONTENT_PART_TEXT);
    assert(message.parts[1].type == DP_CONTENT_PART_IMAGE_BASE64);
    assert(message.parts[2].type == DP_CONTENT_PART_TEXT);
    assert(message.parts[3].type == DP_CONTENT_PART_IMAGE_BASE64);

    dp_free_messages(&message, 1);

    printf("PASS: Inline multimodal context test completed successfully.\n");
    return EXIT_SUCCESS;
}
