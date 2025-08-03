#include "../src/disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main() {
    printf("=== Testing File Attachments Functionality ===\n");
    
    // Test 1: Basic file attachment creation
    printf("Test 1: Basic file attachment creation...\n");
    dp_message_t message = {0};
    message.role = DP_ROLE_USER;
    
    // Add a text part
    if (!dp_message_add_text_part(&message, "Please analyze this file:")) {
        fprintf(stderr, "Failed to add text part\n");
        return 1;
    }
    
    // Add a file data part (simulating a base64-encoded text file)
    const char* test_file_data = "SGVsbG8gV29ybGQh"; // "Hello World!" in base64
    const char* mime_type = "text/plain";
    const char* filename = "test.txt";
    
    if (!dp_message_add_file_data_part(&message, mime_type, test_file_data, filename)) {
        fprintf(stderr, "Failed to add file data part\n");
        dp_free_messages(&message, 1);
        return 1;
    }
    
    assert(message.num_parts == 2);
    assert(message.parts[0].type == DP_CONTENT_PART_TEXT);
    assert(message.parts[1].type == DP_CONTENT_PART_FILE_DATA);
    assert(strcmp(message.parts[1].file_data.mime_type, mime_type) == 0);
    assert(strcmp(message.parts[1].file_data.data, test_file_data) == 0);
    assert(strcmp(message.parts[1].file_data.filename, filename) == 0);
    printf("✓ Basic file attachment creation successful\n");
    
    // Test 2: File attachment without filename
    printf("Test 2: File attachment without filename...\n");
    dp_message_t message2 = {0};
    message2.role = DP_ROLE_USER;
    
    if (!dp_message_add_file_data_part(&message2, "application/pdf", "UERGLTEuNA==", NULL)) {
        fprintf(stderr, "Failed to add file data part without filename\n");
        dp_free_messages(&message, 1);
        return 1;
    }
    
    assert(message2.num_parts == 1);
    assert(message2.parts[0].type == DP_CONTENT_PART_FILE_DATA);
    assert(message2.parts[0].file_data.filename == NULL);
    printf("✓ File attachment without filename successful\n");
    
    // Test 3: Serialization
    printf("Test 3: Serialization...\n");
    char* json_str = NULL;
    if (dp_serialize_messages_to_json_str(&message, 1, &json_str) != 0) {
        fprintf(stderr, "Serialization failed\n");
        dp_free_messages(&message, 1);
        dp_free_messages(&message2, 1);
        return 1;
    }
    
    // Check that the JSON contains expected fields
    assert(strstr(json_str, "file_data") != NULL);
    assert(strstr(json_str, "text/plain") != NULL);
    assert(strstr(json_str, "test.txt") != NULL);
    assert(strstr(json_str, test_file_data) != NULL);
    printf("✓ Serialization successful\n");
    
    // Test 4: Deserialization round-trip
    printf("Test 4: Deserialization round-trip...\n");
    dp_message_t* deserialized_messages = NULL;
    size_t num_messages = 0;
    
    if (dp_deserialize_messages_from_json_str(json_str, &deserialized_messages, &num_messages) != 0) {
        fprintf(stderr, "Deserialization failed\n");
        free(json_str);
        dp_free_messages(&message, 1);
        dp_free_messages(&message2, 1);
        return 1;
    }
    
    assert(num_messages == 1);
    assert(deserialized_messages[0].num_parts == 2);
    assert(deserialized_messages[0].parts[1].type == DP_CONTENT_PART_FILE_DATA);
    assert(strcmp(deserialized_messages[0].parts[1].file_data.mime_type, mime_type) == 0);
    assert(strcmp(deserialized_messages[0].parts[1].file_data.data, test_file_data) == 0);
    assert(strcmp(deserialized_messages[0].parts[1].file_data.filename, filename) == 0);
    printf("✓ Deserialization round-trip successful\n");
    
    // Test 5: Memory cleanup
    printf("Test 5: Memory cleanup...\n");
    dp_free_messages(&message, 1);
    dp_free_messages(&message2, 1);
    dp_free_messages(deserialized_messages, num_messages);
    free(deserialized_messages);
    free(json_str);
    printf("✓ Memory cleanup successful\n");
    
    // Test 6: Multiple file types
    printf("Test 6: Multiple file types...\n");
    dp_message_t multi_message = {0};
    multi_message.role = DP_ROLE_USER;
    
    dp_message_add_text_part(&multi_message, "Analyze these files:");
    dp_message_add_file_data_part(&multi_message, "text/csv", "bmFtZSxhZ2UKSm9obiwyNQ==", "data.csv");
    dp_message_add_file_data_part(&multi_message, "application/json", "eyJrZXkiOiJ2YWx1ZSJ9", "config.json");
    dp_message_add_file_data_part(&multi_message, "image/png", "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8/5+hHgAHggJ/PchI7wAAAABJRU5ErkJggg==", "pixel.png");
    
    assert(multi_message.num_parts == 4);
    assert(multi_message.parts[1].type == DP_CONTENT_PART_FILE_DATA);
    assert(multi_message.parts[2].type == DP_CONTENT_PART_FILE_DATA);
    assert(multi_message.parts[3].type == DP_CONTENT_PART_FILE_DATA);
    
    // Test serialization of multiple files
    char* multi_json = NULL;
    if (dp_serialize_messages_to_json_str(&multi_message, 1, &multi_json) != 0) {
        fprintf(stderr, "Multi-file serialization failed\n");
        dp_free_messages(&multi_message, 1);
        return 1;
    }
    
    assert(strstr(multi_json, "text/csv") != NULL);
    assert(strstr(multi_json, "application/json") != NULL);
    assert(strstr(multi_json, "image/png") != NULL);
    
    dp_free_messages(&multi_message, 1);
    free(multi_json);
    printf("✓ Multiple file types successful\n");
    
    // Test 7: Error conditions
    printf("Test 7: Error conditions...\n");
    dp_message_t error_message = {0};
    error_message.role = DP_ROLE_USER;
    
    // Test with NULL parameters
    assert(!dp_message_add_file_data_part(NULL, "text/plain", "data", "file.txt"));
    assert(!dp_message_add_file_data_part(&error_message, NULL, "data", "file.txt"));
    assert(!dp_message_add_file_data_part(&error_message, "text/plain", NULL, "file.txt"));
    
    printf("✓ Error conditions handled correctly\n");
    
    printf("\n=== All File Attachment Tests Passed! ===\n");
    return 0;
}