#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <disasterparty.h>

static int test_count = 0;
static int test_failures = 0;

#define RUN_TEST(test_func) \
    do { \
        test_count++; \
        printf("Running %s...\n", #test_func); \
        if (test_func() != 0) { \
            test_failures++; \
            printf("FAIL: %s\n", #test_func); \
        } else { \
            printf("PASS: %s\n", #test_func); \
        } \
    } while (0)

#define TEST_IMAGE_MIME_TYPE "image/png"
#define TEST_IMAGE_DATA "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNkYAAAAAYAAjCB0C8AAAAASUVORK5CYII=" // 1x1 transparent PNG

static int test_conversation_serialization_multimodal() {
    dp_message_t original_messages[1];
    dp_message_t* deserialized_messages = NULL;
    size_t num_deserialized_messages = 0;
    char* json_str = NULL;
    int ret = -1;

    // Initialize original message
    memset(&original_messages[0], 0, sizeof(dp_message_t));
    original_messages[0].role = DP_ROLE_USER;
    assert(dp_message_add_text_part(&original_messages[0], "Hello, this is a test."));
    assert(dp_message_add_base64_image_part(&original_messages[0], TEST_IMAGE_MIME_TYPE, TEST_IMAGE_DATA));

    // Serialize messages to JSON string
    ret = dp_serialize_messages_to_json_str(original_messages, 1, &json_str);
    assert(ret == 0);
    assert(json_str != NULL);
    printf("Serialized JSON: %s\n", json_str);

    // Deserialize JSON string back to messages
    ret = dp_deserialize_messages_from_json_str(json_str, &deserialized_messages, &num_deserialized_messages);
    assert(ret == 0);
    assert(deserialized_messages != NULL);
    assert(num_deserialized_messages == 1);

    // Verify deserialized message content
    assert(deserialized_messages[0].role == DP_ROLE_USER);
    assert(deserialized_messages[0].num_parts == 2);

    // Verify text part
    assert(deserialized_messages[0].parts[0].type == DP_CONTENT_PART_TEXT);
    assert(strcmp(deserialized_messages[0].parts[0].text, "Hello, this is a test.") == 0);

    // Verify image part
    assert(deserialized_messages[0].parts[1].type == DP_CONTENT_PART_IMAGE_BASE64);
    assert(strcmp(deserialized_messages[0].parts[1].image_base64.mime_type, TEST_IMAGE_MIME_TYPE) == 0);
    assert(strcmp(deserialized_messages[0].parts[1].image_base64.data, TEST_IMAGE_DATA) == 0);

    // Clean up
    free(json_str);
    dp_free_messages(original_messages, 1);
    dp_free_messages(deserialized_messages, num_deserialized_messages);

    return 0;
}

int main() {
    printf("Starting Multimodal Serialization Tests...\n");
    RUN_TEST(test_conversation_serialization_multimodal);
    printf("Multimodal Serialization Tests finished.\n");

    if (test_failures > 0) {
        printf("Total tests run: %d, Failures: %d\n", test_count, test_failures);
        return 1;
    } else {
        printf("All %d tests passed.\n", test_count);
        return 0;
    }
}