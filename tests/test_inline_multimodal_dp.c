#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h> // For simple, direct checks

// This is a local unit test and does not make network calls.
// It verifies that the message construction helpers build the
// multi-part message in the correct order.

void print_message_structure(const dp_message_t* msg) {
    if (!msg) return;
    printf("Verifying message structure with %zu parts:\n", msg->num_parts);
    for (size_t i = 0; i < msg->num_parts; ++i) {
        printf("  Part %zu: ", i);
        switch (msg->parts[i].type) {
            case DP_CONTENT_PART_TEXT:
                printf("type=text, content=\"%.30s...\"\n", msg->parts[i].text);
                break;
            case DP_CONTENT_PART_IMAGE_URL:
                 printf("type=image_url, url=\"%.30s...\"\n", msg->parts[i].image_url);
                 break;
            case DP_CONTENT_PART_IMAGE_BASE64:
                 printf("type=image_base64, mime=\"%s\"\n", msg->parts[i].image_base64.mime_type);
                 break;
        }
    }
}

int main() {
    printf("Running Inline Multimodal Context Construction Test...\n");
    
    dp_message_t message = {0};
    message.role = DP_ROLE_USER;

    // Build the interleaved message: Text -> Image 1 (URL) -> Text -> Image 2 (Base64)
    bool success = true;
    success &= dp_message_add_text_part(&message, "This is the first text part.");
    success &= dp_message_add_image_url_part(&message, "https://example.com/first_image.png");
    success &= dp_message_add_text_part(&message, "This is the second text part, between the images.");
    success &= dp_message_add_base64_image_part(&message, "image/jpeg", "UjBsR09EbGhjZ0dTQUxNQUFBUUNBRU1tQ1p0dU1GUXhEUzhi");

    if (!success) {
        fprintf(stderr, "FAIL: Failed to build interleaved message.\n");
        dp_free_messages(&message, 1);
        return EXIT_FAILURE;
    }

    print_message_structure(&message);

    // Assertions to verify the structure
    assert(message.num_parts == 4);
    assert(message.parts[0].type == DP_CONTENT_PART_TEXT);
    assert(strcmp(message.parts[0].text, "This is the first text part.") == 0);

    assert(message.parts[1].type == DP_CONTENT_PART_IMAGE_URL);
    assert(strcmp(message.parts[1].image_url, "https://example.com/first_image.png") == 0);

    assert(message.parts[2].type == DP_CONTENT_PART_TEXT);
    assert(strcmp(message.parts[2].text, "This is the second text part, between the images.") == 0);

    assert(message.parts[3].type == DP_CONTENT_PART_IMAGE_BASE64);
    assert(strcmp(message.parts[3].image_base64.mime_type, "image/jpeg") == 0);

    dp_free_messages(&message, 1);

    printf("PASS: Inline multimodal context test completed successfully.\n");
    return EXIT_SUCCESS;
}

