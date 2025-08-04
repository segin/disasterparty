#define _GNU_SOURCE
#include "disasterparty.h"
#include "dp_private.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <ctype.h>
#include <stdint.h>

// Function to get the size of a file
static long dpinternal_get_file_size(FILE *fp) {
    if (!fp) return -1;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return size;
}

// Function to read the entire content of a file into a buffer
static char* dpinternal_read_file_content(const char* path, long* file_size) {
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        perror("Failed to open file");
        return NULL;
    }

    *file_size = dpinternal_get_file_size(fp);
    if (*file_size < 0) {
        fclose(fp);
        return NULL;
    }

    char* buffer = malloc(*file_size);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate memory for file content\n");
        fclose(fp);
        return NULL;
    }

    if (fread(buffer, 1, *file_size, fp) != (size_t)*file_size) {
        fprintf(stderr, "Failed to read entire file\n");
        free(buffer);
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    return buffer;
}

// Extracts the filename from a full path
static const char* dpinternal_get_filename_from_path(const char* path) {
    const char* filename = strrchr(path, '/');
    if (filename) {
        return filename + 1;
    } else {
        return path; // No slash found, treat the whole path as the filename
    }
}

// Simple MIME type detection based on file extension
static const char* dpinternal_detect_mime_type(const char* filename) {
    if (!filename) return "application/octet-stream";
    
    const char* ext = strrchr(filename, '.');
    if (!ext) return "application/octet-stream";
    
    // Convert extension to lowercase for comparison
    char lower_ext[32];
    size_t ext_len = strlen(ext);
    if (ext_len >= sizeof(lower_ext)) return "application/octet-stream";
    
    for (size_t i = 0; i < ext_len; i++) {
        lower_ext[i] = tolower(ext[i]);
    }
    lower_ext[ext_len] = '\0';
    
    // Common file type mappings
    if (strcmp(lower_ext, ".txt") == 0) return "text/plain";
    if (strcmp(lower_ext, ".html") == 0 || strcmp(lower_ext, ".htm") == 0) return "text/html";
    if (strcmp(lower_ext, ".css") == 0) return "text/css";
    if (strcmp(lower_ext, ".js") == 0) return "application/javascript";
    if (strcmp(lower_ext, ".json") == 0) return "application/json";
    if (strcmp(lower_ext, ".xml") == 0) return "application/xml";
    if (strcmp(lower_ext, ".pdf") == 0) return "application/pdf";
    if (strcmp(lower_ext, ".doc") == 0) return "application/msword";
    if (strcmp(lower_ext, ".docx") == 0) return "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    if (strcmp(lower_ext, ".xls") == 0) return "application/vnd.ms-excel";
    if (strcmp(lower_ext, ".xlsx") == 0) return "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    if (strcmp(lower_ext, ".ppt") == 0) return "application/vnd.ms-powerpoint";
    if (strcmp(lower_ext, ".pptx") == 0) return "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    if (strcmp(lower_ext, ".zip") == 0) return "application/zip";
    if (strcmp(lower_ext, ".tar") == 0) return "application/x-tar";
    if (strcmp(lower_ext, ".gz") == 0) return "application/gzip";
    if (strcmp(lower_ext, ".jpg") == 0 || strcmp(lower_ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(lower_ext, ".png") == 0) return "image/png";
    if (strcmp(lower_ext, ".gif") == 0) return "image/gif";
    if (strcmp(lower_ext, ".bmp") == 0) return "image/bmp";
    if (strcmp(lower_ext, ".svg") == 0) return "image/svg+xml";
    if (strcmp(lower_ext, ".mp3") == 0) return "audio/mpeg";
    if (strcmp(lower_ext, ".wav") == 0) return "audio/wav";
    if (strcmp(lower_ext, ".mp4") == 0) return "video/mp4";
    if (strcmp(lower_ext, ".avi") == 0) return "video/x-msvideo";
    if (strcmp(lower_ext, ".mov") == 0) return "video/quicktime";
    if (strcmp(lower_ext, ".csv") == 0) return "text/csv";
    if (strcmp(lower_ext, ".md") == 0) return "text/markdown";
    if (strcmp(lower_ext, ".c") == 0 || strcmp(lower_ext, ".h") == 0) return "text/x-c";
    if (strcmp(lower_ext, ".cpp") == 0 || strcmp(lower_ext, ".cc") == 0 || strcmp(lower_ext, ".cxx") == 0) return "text/x-c++";
    if (strcmp(lower_ext, ".py") == 0) return "text/x-python";
    if (strcmp(lower_ext, ".java") == 0) return "text/x-java";
    if (strcmp(lower_ext, ".sh") == 0) return "application/x-sh";
    
    return "application/octet-stream";
}

// Base64 encoding function for file data
static char* dpinternal_base64_encode(const unsigned char* data, size_t input_length) {
    static const char encoding_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static const int mod_table[] = {0, 2, 1};

    size_t output_length = 4 * ((input_length + 2) / 3);
    char* encoded_data = malloc(output_length + 1);
    if (!encoded_data) return NULL;

    for (size_t i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[output_length - 1 - i] = '=';

    encoded_data[output_length] = '\0';
    return encoded_data;
}

// Validate file data part parameters
bool dpinternal_validate_file_data_part(const char* mime_type, const char* base64_data, const char* filename) {
    // mime_type and base64_data are required, filename is optional
    if (!mime_type || !base64_data) {
        return false;
    }
    
    // Basic validation - check that strings are not empty
    if (strlen(mime_type) == 0 || strlen(base64_data) == 0) {
        return false;
    }
    
    // Optional: validate base64 format (basic check)
    size_t data_len = strlen(base64_data);
    if (data_len % 4 != 0) {
        return false; // Base64 should be multiple of 4
    }
    
    return true;
}

// Helper function to read a file and encode it as base64
char* dpinternal_encode_file_to_base64(const char* file_path) {
    if (!file_path) return NULL;
    
    long file_size;
    char* file_content = dpinternal_read_file_content(file_path, &file_size);
    if (!file_content) return NULL;
    
    char* base64_data = dpinternal_base64_encode((unsigned char*)file_content, file_size);
    free(file_content);
    
    return base64_data;
}

// Helper function to add a file from filesystem to a message
bool dpinternal_message_add_file_from_path(dp_message_t* message, const char* file_path, const char* mime_type_override) {
    if (!message || !file_path) return false;
    
    // Detect MIME type if not provided
    const char* mime_type = mime_type_override ? mime_type_override : dpinternal_detect_mime_type(file_path);
    
    // Encode file to base64
    char* base64_data = dpinternal_encode_file_to_base64(file_path);
    if (!base64_data) return false;
    
    // Extract filename from path
    const char* filename = dpinternal_get_filename_from_path(file_path);
    
    // Add the file data part to the message
    bool success = dp_message_add_file_data_part(message, mime_type, base64_data, filename);
    
    free(base64_data);
    return success;
}