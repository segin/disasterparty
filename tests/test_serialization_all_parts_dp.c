#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Helper to compare two dp_message_t arrays
bool compare_messages(const dp_message_t* msg1, size_t num_msg1, const dp_message_t* msg2, size_t num_msg2) {
    if (num_msg1 != num_msg2) {
        fprintf(stderr, "Mismatch in number of messages: %zu vs %zu\n", num_msg1, num_msg2);
        return false;
    }

    for (size_t i = 0; i < num_msg1; ++i) {
        if (msg1[i].role != msg2[i].role) {
            fprintf(stderr, "Mismatch in message role at index %zu\n", i);
            return false;
        }
        if (msg1[i].num_parts != msg2[i].num_parts) {
            fprintf(stderr, "Mismatch in number of parts for message %zu: %zu vs %zu\n", i, msg1[i].num_parts, msg2[i].num_parts);
            return false;
        }

        for (size_t j = 0; j < msg1[i].num_parts; ++j) {
            const dp_content_part_t* part1 = &msg1[i].parts[j];
            const dp_content_part_t* part2 = &msg2[i].parts[j];

            if (part1->type != part2->type) {
                fprintf(stderr, "Mismatch in part type for message %zu, part %zu\n", i, j);
                return false;
            }

            switch (part1->type) {
                case DP_CONTENT_PART_TEXT:
                    if (strcmp(part1->text, part2->text) != 0) {
                        fprintf(stderr, "Mismatch in text content for message %zu, part %zu\n", i, j);
                        return false;
                    }
                    break;
                case DP_CONTENT_PART_IMAGE_URL:
                    if (strcmp(part1->image_url, part2->image_url) != 0) {
                        fprintf(stderr, "Mismatch in image URL for message %zu, part %zu\n", i, j);
                        return false;
                    }
                    break;        
        case DP_CONTENT_PART_IMAGE_BASE64:
                    if (strcmp(part1->image_base64.mime_type, part2->image_base64.mime_type) != 0 ||
                        strcmp(part1->image_base64.data, part2->image_base64.data) != 0) {
                        fprintf(stderr, "Mismatch in base64 image data for message %zu, part %zu\n", i, j);
                        return false;
                    }
                    break;
                case DP_CONTENT_PART_FILE_DATA:
                    if (strcmp(part1->file_data.mime_type, part2->file_data.mime_type) != 0 ||
                        strcmp(part1->file_data.data, part2->file_data.data) != 0) {
                        fprintf(stderr, "Mismatch in file data for message %zu, part %zu\n", i, j);
                        return false;
                    }
                    if ((part1->file_data.filename == NULL) != (part2->file_data.filename == NULL)) {
                        fprintf(stderr, "Mismatch in file data filename presence for message %zu, part %zu\n", i, j);
                        return false;
                    }
                    if (part1->file_data.filename && strcmp(part1->file_data.filename, part2->file_data.filename) != 0) {
                        fprintf(stderr, "Mismatch in file data filename for message %zu, part %zu\n", i, j);
                        return false;
                    }
                    break;
                case DP_CONTENT_PART_FILE_REFERENCE:
                    if (strcmp(part1->file_reference.file_id, part2->file_reference.file_id) != 0 ||
                        strcmp(part1->file_reference.mime_type, part2->file_reference.mime_type) != 0) {
                        fprintf(stderr, "Mismatch in file reference data for message %zu, part %zu\n", i, j);
                        return false;
                    }
                    break;
            }
        }
    }
    return true;
}

int main() {
    printf("Testing serialization/deserialization with all content part types...\n");

    dp_message_t original_messages[2];
    memset(original_messages, 0, sizeof(original_messages));

    // Message 1: Text and Image URL
    original_messages[0].role = DP_ROLE_USER;
    dp_message_add_text_part(&original_messages[0], "Hello, this is a test message.");
    dp_message_add_image_url_part(&original_messages[0], "https://example.com/image.png");

    // Message 2: Base64 Image and File Reference
    original_messages[1].role = DP_ROLE_ASSISTANT;
    dp_message_add_base64_image_part(&original_messages[1], "image/jpeg", "QUJDREVGR0hJSktMTU5PUFFSU1RVVldYWVo=");
    dp_message_add_file_reference_part(&original_messages[1], "file-123", "application/pdf");

    const char* test_file_path = "./all_parts_conversation.json";

    // 1. Serialize messages to file
    int ret = dp_serialize_messages_to_file(original_messages, 2, test_file_path);
    if (ret != 0) {
        fprintf(stderr, "FAILURE: Failed to serialize messages to file (error code: %d).\n", ret);
        dp_free_messages(original_messages, 2);
        return EXIT_FAILURE;
    }

    // 2. Deserialize messages from file
    dp_message_t* deserialized_messages = NULL;
    size_t num_deserialized_messages = 0;
    ret = dp_deserialize_messages_from_file(test_file_path, &deserialized_messages, &num_deserialized_messages);
    if (ret != 0) {
        fprintf(stderr, "FAILURE: Failed to deserialize messages from file (error code: %d).\n", ret);
        dp_free_messages(original_messages, 2);
        remove(test_file_path);
        return EXIT_FAILURE;
    }

    // 3. Compare original and deserialized messages
    if (compare_messages(original_messages, 2, deserialized_messages, num_deserialized_messages)) {
        printf("SUCCESS: Serialization and deserialization with all content parts passed.\n");
        ret = EXIT_SUCCESS;
    } else {
        fprintf(stderr, "FAILURE: Original and deserialized messages do not match.\n");
        ret = EXIT_FAILURE;
    }

    // Cleanup
    dp_free_messages(original_messages, 2);
    dp_free_messages(deserialized_messages, num_deserialized_messages);
    free(deserialized_messages);
    remove(test_file_path);

    return ret;
}