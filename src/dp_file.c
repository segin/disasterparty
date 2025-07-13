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

int dp_upload_file(dp_context_t* context, const char* file_path, const char* mime_type, dp_file_t** file_out) {
    if (!context || !file_path || !file_out) {
        if (file_out) *file_out = NULL;
        return DP_ERROR_INVALID_ARGUMENT;
    }

    if (context->provider != DP_PROVIDER_GOOGLE_GEMINI) {
        return DP_ERROR_PROVIDER_FAILURE; // Or a more specific error
    }

    *file_out = calloc(1, sizeof(dp_file_t));
    if (!*file_out) {
        return DP_ERROR_GENERAL;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        free(*file_out);
        *file_out = NULL;
        return DP_ERROR_GENERAL;
    }

    long file_size;
    char* file_content = read_file_content(file_path, &file_size);
    if (!file_content) {
        free(*file_out);
        *file_out = NULL;
        curl_easy_cleanup(curl);
        return DP_ERROR_GENERAL;
    }

    char url[1024];
    snprintf(url, sizeof(url), "%s/files?key=%s", context->api_base_url, context->api_key);

    struct curl_slist* headers = NULL;
    char content_type_header[256];
    snprintf(content_type_header, sizeof(content_type_header), "Content-Type: %s", mime_type);
    headers = curl_slist_append(headers, content_type_header);

    char x_goog_file_name_header[1024];
    snprintf(x_goog_file_name_header, sizeof(x_goog_file_name_header), "x-goog-file-name: %s", get_filename_from_path(file_path));
    headers = curl_slist_append(headers, x_goog_file_name_header);

    memory_struct_t chunk_mem = { .memory = malloc(1), .size = 0 };
    if (!chunk_mem.memory) {
        free(file_content);
        free(*file_out);
        *file_out = NULL;
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return DP_ERROR_GENERAL;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, file_content);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, file_size);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk_mem);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, dp_get_user_agent(context));

    CURLcode res = curl_easy_perform(curl);
    long http_status_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status_code);

    int return_code = DP_SUCCESS;

    if (res != CURLE_OK || http_status_code < 200 || http_status_code >= 300) {
        // Error handling
        return_code = DP_ERROR_NETWORK;
    } else {
        cJSON *root = cJSON_Parse(chunk_mem.memory);
        if (root) {
            cJSON* file_obj = cJSON_GetObjectItemCaseSensitive(root, "file");
            if (file_obj) {
                // Parsing logic
            }
            cJSON_Delete(root);
        } else {
            return_code = DP_ERROR_GENERAL;
        }
    }

    free(file_content);
    free(chunk_mem.memory);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (return_code != DP_SUCCESS) {
        dp_free_file(*file_out);
        *file_out = NULL;
    }

    return return_code;
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
