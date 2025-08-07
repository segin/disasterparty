#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("Testing dp_deserialize_messages_from_file with a malformed JSON file...\n");

    const char* malformed_json_file_path = "./malformed_conversation.json";
    // Missing a brace and a bracket
    const char* malformed_json_content = "[ { \"role\": \"user\", \"parts\": [ { \"type\": \"text\", \"text\": \"Hello\" } ] }, { \"role\": \"assistant\", \"parts\": [ { \"type\": \"text\", \"text\": \"World\" } } ] ";

    // Create a file with malformed JSON content
    FILE* fp = fopen(malformed_json_file_path, "w");
    if (fp == NULL) {
        fprintf(stderr, "Failed to create malformed JSON file.\n");
        return EXIT_FAILURE;
    }
    fputs(malformed_json_content, fp);
    fclose(fp);

    dp_message_t* deserialized_messages = NULL;
    size_t num_deserialized_messages = 0;

    // Attempt to deserialize the malformed file
    int ret = dp_deserialize_messages_from_file(malformed_json_file_path, &deserialized_messages, &num_deserialized_messages);

    // Assert that deserialization failed and no messages were returned
    if (ret != 0 && deserialized_messages == NULL && num_deserialized_messages == 0) {
        printf("SUCCESS: dp_deserialize_messages_from_file correctly failed for a malformed JSON file.\n");
        ret = EXIT_SUCCESS;
    } else {
        fprintf(stderr, "FAILURE: dp_deserialize_messages_from_file did not fail as expected for malformed JSON.\n");
        if (deserialized_messages) {
            fprintf(stderr, "  Deserialized messages were not NULL.\n");
            dp_free_messages(deserialized_messages, num_deserialized_messages);
        }
        if (num_deserialized_messages != 0) {
            fprintf(stderr, "  Number of deserialized messages was not 0 (got %zu).\n", num_deserialized_messages);
        }
        ret = EXIT_FAILURE;
    }

    // Cleanup
    remove(malformed_json_file_path);

    return ret;
}