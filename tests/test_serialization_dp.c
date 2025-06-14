#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

void print_messages(const char* header, const dp_message_t* messages, size_t num_messages) {
    printf("--- %s ---\n", header);
    for (size_t i = 0; i < num_messages; ++i) {
        const char* role = "UNKNOWN";
        switch(messages[i].role) {
            case DP_ROLE_SYSTEM: role = "system"; break;
            case DP_ROLE_USER: role = "user"; break;
            case DP_ROLE_ASSISTANT: role = "assistant"; break;
            case DP_ROLE_TOOL: role = "tool"; break;
        }
        printf("Message %zu (Role: %s):\n", i, role);
        for (size_t j = 0; j < messages[i].num_parts; ++j) {
            const dp_content_part_t* part = &messages[i].parts[j];
            printf("  Part %zu: ", j);
            switch(part->type) {
                case DP_CONTENT_PART_TEXT:
                    printf("type=text, text=\"%s\"\n", part->text ? part->text : "(null)");
                    break;
                case DP_CONTENT_PART_IMAGE_URL:
                    printf("type=image_url, url=\"%s\"\n", part->image_url ? part->image_url : "(null)");
                    break;
                case DP_CONTENT_PART_IMAGE_BASE64:
                    printf("type=image_base64, mime=\"%s\", data=\"%.10s...\"\n",
                           part->image_base64.mime_type ? part->image_base64.mime_type : "(null)",
                           part->image_base64.data ? part->image_base64.data : "(null)");
                    break;
            }
        }
    }
    printf("-------------------------\n\n");
}


int main() {
    printf("Running Disaster Party Serialization Test...\n");

    // 1. Create a sample conversation
    dp_message_t messages[2];
    memset(messages, 0, sizeof(messages));
    size_t num_messages = 2;

    messages[0].role = DP_ROLE_USER;
    dp_message_add_text_part(&messages[0], "Hello, this is a test.");
    dp_message_add_image_url_part(&messages[0], "https://example.com/image.png");

    messages[1].role = DP_ROLE_ASSISTANT;
    dp_message_add_text_part(&messages[1], "I am responding to your test.");

    print_messages("Original Messages", messages, num_messages);

    // 2. Serialize to JSON string
    char* json_string = NULL;
    if (dp_serialize_messages_to_json_str(messages, num_messages, &json_string) != 0) {
        fprintf(stderr, "FAIL: dp_serialize_messages_to_json_str failed.\n");
        dp_free_messages(messages, num_messages);
        return EXIT_FAILURE;
    }
    printf("Serialized to JSON String:\n%s\n", json_string);
    assert(json_string != NULL);
    printf("PASS: Serialization to JSON string successful.\n\n");

    // 3. Deserialize from JSON string
    dp_message_t* deserialized_messages = NULL;
    size_t num_deserialized = 0;
    if (dp_deserialize_messages_from_json_str(json_string, &deserialized_messages, &num_deserialized) != 0) {
        fprintf(stderr, "FAIL: dp_deserialize_messages_from_json_str failed.\n");
        free(json_string);
        dp_free_messages(messages, num_messages);
        return EXIT_FAILURE;
    }
    assert(deserialized_messages != NULL);
    assert(num_deserialized == num_messages);
    print_messages("Deserialized Messages from String", deserialized_messages, num_deserialized);
    printf("PASS: Deserialization from JSON string successful.\n\n");

    // Verification
    assert(deserialized_messages[0].role == DP_ROLE_USER);
    assert(deserialized_messages[0].num_parts == 2);
    assert(strcmp(deserialized_messages[0].parts[0].text, "Hello, this is a test.") == 0);
    assert(strcmp(deserialized_messages[0].parts[1].image_url, "https://example.com/image.png") == 0);
    assert(deserialized_messages[1].role == DP_ROLE_ASSISTANT);
    assert(deserialized_messages[1].num_parts == 1);

    dp_free_messages(deserialized_messages, num_deserialized);
    free(deserialized_messages);

    // 4. Serialize to File
    const char* test_file = "conversation.json";
    if (dp_serialize_messages_to_file(messages, num_messages, test_file) != 0) {
        fprintf(stderr, "FAIL: dp_serialize_messages_to_file failed.\n");
        free(json_string);
        dp_free_messages(messages, num_messages);
        return EXIT_FAILURE;
    }
    printf("PASS: Serialization to file '%s' successful.\n\n", test_file);
    free(json_string);

    // 5. Deserialize from File
    deserialized_messages = NULL;
    num_deserialized = 0;
    if (dp_deserialize_messages_from_file(test_file, &deserialized_messages, &num_deserialized) != 0) {
        fprintf(stderr, "FAIL: dp_deserialize_messages_from_file failed.\n");
        dp_free_messages(messages, num_messages);
        return EXIT_FAILURE;
    }
    assert(deserialized_messages != NULL);
    assert(num_deserialized == num_messages);
    print_messages("Deserialized Messages from File", deserialized_messages, num_deserialized);
    printf("PASS: Deserialization from file successful.\n\n");

    // Clean up original messages
    dp_free_messages(messages, num_messages);
    // Clean up deserialized messages
    dp_free_messages(deserialized_messages, num_deserialized);
    free(deserialized_messages);

    remove(test_file); // Clean up the test file

    printf("All serialization tests passed!\n");

    return EXIT_SUCCESS;
}
