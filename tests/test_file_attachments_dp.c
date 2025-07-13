#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main() {
    printf("Running File Attachment Test...\n");

    const char* pdf_path = getenv("TEST_PDF_PATH");
    if (!pdf_path) {
        printf("SKIP: TEST_PDF_PATH environment variable not set. Please set it to the path of a PDF file.\n");
        return 77;
    }

    dp_message_t message = {0};
    message.role = DP_ROLE_USER;

    // Add a file reference part to the message
    if (!dp_message_add_file_reference_part(&message, "file-1234", "application/pdf")) {
        fprintf(stderr, "Failed to add file reference part to message.\n");
        return EXIT_FAILURE;
    }

    printf("Successfully built message with file attachment.\n");

    // Serialize the message to a JSON string to verify its structure
    char* json_str = NULL;
    if (dp_serialize_messages_to_json_str(&message, 1, &json_str) != 0) {
        fprintf(stderr, "Failed to serialize message to JSON string.\n");
        dp_free_messages(&message, 1);
        return EXIT_FAILURE;
    }

    printf("Serialized JSON: %s\n", json_str);

    // This test doesn't call an API, it just verifies the message structure can be built.
    assert(message.num_parts == 1);
    assert(message.parts[0].type == DP_CONTENT_PART_FILE_REFERENCE);
    assert(strcmp(message.parts[0].file_reference.file_id, "file-1234") == 0);
    assert(strcmp(message.parts[0].file_reference.mime_type, "application/pdf") == 0);

    dp_free_messages(&message, 1);
    free(json_str);

    printf("PASS: File attachment test completed successfully.\n");
    return EXIT_SUCCESS;
}
