#include "disasterparty.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>

// Helper function to create a test file
static int create_test_file(const char* filename, const char* content) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        return -1;
    }
    fprintf(file, "%s", content);
    fclose(file);
    return 0;
}

// Helper function to encode file to base64 (simple implementation for testing)
static char* encode_file_to_base64(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Read file content
    char* buffer = malloc(file_size);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    if (fread(buffer, 1, file_size, file) != (size_t)file_size) {
        free(buffer);
        fclose(file);
        return NULL;
    }
    fclose(file);
    
    // Simple base64 encoding (for testing purposes)
    // In a real implementation, you'd use a proper base64 encoder
    static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t encoded_len = 4 * ((file_size + 2) / 3);
    char* encoded = malloc(encoded_len + 1);
    if (!encoded) {
        free(buffer);
        return NULL;
    }
    
    size_t i, j;
    for (i = 0, j = 0; i < file_size;) {
        uint32_t octet_a = i < file_size ? (unsigned char)buffer[i++] : 0;
        uint32_t octet_b = i < file_size ? (unsigned char)buffer[i++] : 0;
        uint32_t octet_c = i < file_size ? (unsigned char)buffer[i++] : 0;
        
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        
        encoded[j++] = base64_chars[(triple >> 3 * 6) & 0x3F];
        encoded[j++] = base64_chars[(triple >> 2 * 6) & 0x3F];
        encoded[j++] = base64_chars[(triple >> 1 * 6) & 0x3F];
        encoded[j++] = base64_chars[(triple >> 0 * 6) & 0x3F];
    }
    
    // Add padding
    for (i = 0; i < (3 - file_size % 3) % 3; i++) {
        encoded[encoded_len - 1 - i] = '=';
    }
    
    encoded[encoded_len] = '\0';
    free(buffer);
    return encoded;
}

int main() {
    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Testing file attachments functionality...\n");
    
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed.\n");
        return EXIT_FAILURE;
    }
    
    int test_failures = 0;
    
    // Test 1: Basic file attachment functionality
    printf("\n=== Test 1: Basic file attachment functionality ===\n");
    
    // Create test files
    const char* test_txt_file = "test_file.txt";
    const char* test_csv_file = "test_file.csv";
    const char* test_pdf_file = "test_file.pdf";
    
    if (create_test_file(test_txt_file, "This is a test text file for file attachments.") != 0) {
        fprintf(stderr, "Failed to create test text file.\n");
        test_failures++;
    }
    
    if (create_test_file(test_csv_file, "name,age,city\nJohn,30,New York\nJane,25,Los Angeles") != 0) {
        fprintf(stderr, "Failed to create test CSV file.\n");
        test_failures++;
    }
    
    if (create_test_file(test_pdf_file, "%PDF-1.4\n1 0 obj\n<<\n/Type /Catalog\n/Pages 2 0 R\n>>\nendobj\n") != 0) {
        fprintf(stderr, "Failed to create test PDF file.\n");
        test_failures++;
    }
    
    // Test message creation with file attachments
    dp_message_t message = {0};
    message.role = DP_ROLE_USER;
    
    // Test text file attachment
    char* txt_base64 = encode_file_to_base64(test_txt_file);
    if (txt_base64) {
        if (dp_message_add_file_data_part(&message, "text/plain", txt_base64, "test_file.txt")) {
            printf("✓ Successfully added text file attachment\n");
        } else {
            printf("✗ Failed to add text file attachment\n");
            test_failures++;
        }
        free(txt_base64);
    } else {
        printf("✗ Failed to encode text file to base64\n");
        test_failures++;
    }
    
    // Test CSV file attachment
    char* csv_base64 = encode_file_to_base64(test_csv_file);
    if (csv_base64) {
        if (dp_message_add_file_data_part(&message, "text/csv", csv_base64, "test_file.csv")) {
            printf("✓ Successfully added CSV file attachment\n");
        } else {
            printf("✗ Failed to add CSV file attachment\n");
            test_failures++;
        }
        free(csv_base64);
    } else {
        printf("✗ Failed to encode CSV file to base64\n");
        test_failures++;
    }
    
    // Test PDF file attachment
    char* pdf_base64 = encode_file_to_base64(test_pdf_file);
    if (pdf_base64) {
        if (dp_message_add_file_data_part(&message, "application/pdf", pdf_base64, "test_file.pdf")) {
            printf("✓ Successfully added PDF file attachment\n");
        } else {
            printf("✗ Failed to add PDF file attachment\n");
            test_failures++;
        }
        free(pdf_base64);
    } else {
        printf("✗ Failed to encode PDF file to base64\n");
        test_failures++;
    }
    
    // Test 2: Error conditions
    printf("\n=== Test 2: Error conditions ===\n");
    
    // Test with NULL parameters
    if (!dp_message_add_file_data_part(NULL, "text/plain", "dGVzdA==", "test.txt")) {
        printf("✓ Correctly rejected NULL message parameter\n");
    } else {
        printf("✗ Should have rejected NULL message parameter\n");
        test_failures++;
    }
    
    if (!dp_message_add_file_data_part(&message, NULL, "dGVzdA==", "test.txt")) {
        printf("✓ Correctly rejected NULL mime_type parameter\n");
    } else {
        printf("✗ Should have rejected NULL mime_type parameter\n");
        test_failures++;
    }
    
    if (!dp_message_add_file_data_part(&message, "text/plain", NULL, "test.txt")) {
        printf("✓ Correctly rejected NULL base64_data parameter\n");
    } else {
        printf("✗ Should have rejected NULL base64_data parameter\n");
        test_failures++;
    }
    
    // Test with empty strings (these should be accepted as valid but empty data)
    if (dp_message_add_file_data_part(&message, "", "dGVzdA==", "test.txt")) {
        printf("✓ Correctly accepted empty mime_type (valid but unusual)\n");
    } else {
        printf("✗ Failed to accept empty mime_type\n");
        test_failures++;
    }
    
    if (dp_message_add_file_data_part(&message, "text/plain", "", "test.txt")) {
        printf("✓ Correctly accepted empty base64_data (valid but unusual)\n");
    } else {
        printf("✗ Failed to accept empty base64_data\n");
        test_failures++;
    }
    
    // Test 3: Memory management
    printf("\n=== Test 3: Memory management ===\n");
    
    // Verify that parts were allocated correctly (3 original + 2 empty string tests = 5)
    if (message.num_parts == 5) {
        printf("✓ Correct number of parts allocated (%zu)\n", message.num_parts);
        
        // Check each part
        for (size_t i = 0; i < message.num_parts; i++) {
            if (message.parts[i].type == DP_CONTENT_PART_FILE_DATA) {
                if (message.parts[i].file_data.mime_type && 
                    message.parts[i].file_data.data && 
                    message.parts[i].file_data.filename) {
                    printf("✓ Part %zu has all required file data fields\n", i);
                } else {
                    printf("✗ Part %zu missing required file data fields\n", i);
                    test_failures++;
                }
            } else {
                printf("✗ Part %zu has incorrect type (expected DP_CONTENT_PART_FILE_DATA)\n", i);
                test_failures++;
            }
        }
    } else {
        printf("✗ Incorrect number of parts (%zu, expected 5)\n", message.num_parts);
        test_failures++;
    }
    
    // Test 4: Mixed content types
    printf("\n=== Test 4: Mixed content types ===\n");
    
    dp_message_t mixed_message = {0};
    mixed_message.role = DP_ROLE_USER;
    
    // Add text part
    if (dp_message_add_text_part(&mixed_message, "Please analyze this file:")) {
        printf("✓ Successfully added text part to mixed message\n");
    } else {
        printf("✗ Failed to add text part to mixed message\n");
        test_failures++;
    }
    
    // Add file part
    char* mixed_base64 = encode_file_to_base64(test_txt_file);
    if (mixed_base64) {
        if (dp_message_add_file_data_part(&mixed_message, "text/plain", mixed_base64, "analysis.txt")) {
            printf("✓ Successfully added file part to mixed message\n");
        } else {
            printf("✗ Failed to add file part to mixed message\n");
            test_failures++;
        }
        free(mixed_base64);
    }
    
    // Verify mixed message structure
    if (mixed_message.num_parts == 2) {
        if (mixed_message.parts[0].type == DP_CONTENT_PART_TEXT &&
            mixed_message.parts[1].type == DP_CONTENT_PART_FILE_DATA) {
            printf("✓ Mixed message has correct part types and order\n");
        } else {
            printf("✗ Mixed message has incorrect part types or order\n");
            test_failures++;
        }
    } else {
        printf("✗ Mixed message has incorrect number of parts (%zu, expected 2)\n", mixed_message.num_parts);
        test_failures++;
    }
    
    // Clean up
    printf("\n=== Cleanup ===\n");
    
    dp_message_t messages[] = {message, mixed_message};
    dp_free_messages(messages, 2);
    printf("✓ Messages freed successfully\n");
    
    // Remove test files
    unlink(test_txt_file);
    unlink(test_csv_file);
    unlink(test_pdf_file);
    printf("✓ Test files cleaned up\n");
    
    curl_global_cleanup();
    
    // Final results
    printf("\n=== Test Results ===\n");
    if (test_failures == 0) {
        printf("✓ All file attachment tests passed!\n");
        printf("File attachments test (Disaster Party) finished successfully.\n");
        return EXIT_SUCCESS;
    } else {
        printf("✗ %d test(s) failed\n", test_failures);
        printf("File attachments test (Disaster Party) finished with failures.\n");
        return EXIT_FAILURE;
    }
}