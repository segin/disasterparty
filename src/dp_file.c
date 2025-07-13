#include "disasterparty.h"
#include "dp_private.h"
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Function to get the size of a file
static long get_file_size(FILE *fp) {
    if (!fp) return -1;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return size;
}

// Function to read the entire content of a file into a buffer
static char* read_file_content(const char* path, long* file_size) {
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        perror("Failed to open file");
        return NULL;
    }

    *file_size = get_file_size(fp);
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
static const char* get_filename_from_path(const char* path) {
    const char* filename = strrchr(path, '/');
    if (filename) {
        return filename + 1;
    } else {
        return path; // No slash found, treat the whole path as the filename
    }
}



void dp_free_file(dp_file_t* file) {
    if (!file) return;
    free(file->file_id);
    free(file->display_name);
    free(file->mime_type);
    free(file->create_time);
    free(file->uri);
    free(file);
}
