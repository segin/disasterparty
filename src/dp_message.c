#define _GNU_SOURCE
#include "disasterparty.h"
#include "dp_private.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void dp_free_messages(dp_message_t* messages, size_t num_messages) {
    if (!messages) return;
    for (size_t i = 0; i < num_messages; ++i) {
        if (messages[i].parts) {
            for (size_t j = 0; j < messages[i].num_parts; ++j) {
                dp_content_part_t* part = &messages[i].parts[j];
                free(part->text);
                free(part->image_url);
                if (part->type == DP_CONTENT_PART_IMAGE_BASE64) {
                    free(part->image_base64.mime_type);
                    free(part->image_base64.data);
                } else if (part->type == DP_CONTENT_PART_FILE_DATA) {
                    free(part->file_data.mime_type);
                    free(part->file_data.data);
                    free(part->file_data.filename);
                }
            }
            free(messages[i].parts);
            messages[i].parts = NULL; 
            messages[i].num_parts = 0;
        }
    }
}

bool dpinternal_message_add_part_internal(dp_message_t* message, dp_content_part_type_t type,
                                   const char* text_content, const char* image_url_content,
                                   const char* mime_type_content, const char* base64_data_content,
                                   const char* filename_content) {
    if (!message) return false;
    dp_content_part_t* new_parts_array = realloc(message->parts, (message->num_parts + 1) * sizeof(dp_content_part_t));
    if (!new_parts_array) return false;
    message->parts = new_parts_array;

    dp_content_part_t* new_part = &message->parts[message->num_parts];
    memset(new_part, 0, sizeof(dp_content_part_t));
    new_part->type = type;
    bool success = false;

    if (type == DP_CONTENT_PART_TEXT && text_content) {
        new_part->text = dpinternal_strdup(text_content);
        success = (new_part->text != NULL);
    } else if (type == DP_CONTENT_PART_IMAGE_URL && image_url_content) {
        new_part->image_url = dpinternal_strdup(image_url_content);
        success = (new_part->image_url != NULL);
    } else if (type == DP_CONTENT_PART_IMAGE_BASE64 && mime_type_content && base64_data_content) {
        new_part->image_base64.mime_type = dpinternal_strdup(mime_type_content);
        new_part->image_base64.data = dpinternal_strdup(base64_data_content);
        success = (new_part->image_base64.mime_type != NULL && new_part->image_base64.data != NULL);
        if (!success) {
            free(new_part->image_base64.mime_type); new_part->image_base64.mime_type = NULL;
            free(new_part->image_base64.data); new_part->image_base64.data = NULL;
        }
    } else if (type == DP_CONTENT_PART_FILE_DATA && mime_type_content && base64_data_content) {
        new_part->file_data.mime_type = dpinternal_strdup(mime_type_content);
        new_part->file_data.data = dpinternal_strdup(base64_data_content);
        new_part->file_data.filename = filename_content ? dpinternal_strdup(filename_content) : NULL;
        success = (new_part->file_data.mime_type != NULL && new_part->file_data.data != NULL);
        if (!success) {
            free(new_part->file_data.mime_type); new_part->file_data.mime_type = NULL;
            free(new_part->file_data.data); new_part->file_data.data = NULL;
            free(new_part->file_data.filename); new_part->file_data.filename = NULL;
        }
    }
    if (success) message->num_parts++;
    else fprintf(stderr, "Failed to allocate memory for Disaster Party message content part.\n");
    return success;
}

bool dp_message_add_text_part(dp_message_t* message, const char* text) {
    return dpinternal_message_add_part_internal(message, DP_CONTENT_PART_TEXT, text, NULL, NULL, NULL, NULL);
}

bool dp_message_add_image_url_part(dp_message_t* message, const char* image_url) {
    return dpinternal_message_add_part_internal(message, DP_CONTENT_PART_IMAGE_URL, NULL, image_url, NULL, NULL, NULL);
}

bool dp_message_add_base64_image_part(dp_message_t* message, const char* mime_type, const char* base64_data) {
    return dpinternal_message_add_part_internal(message, DP_CONTENT_PART_IMAGE_BASE64, NULL, NULL, mime_type, base64_data, NULL);
}

bool dp_message_add_file_data_part(dp_message_t* message, const char* mime_type, const char* base64_data, const char* filename) {
    return dpinternal_message_add_part_internal(message, DP_CONTENT_PART_FILE_DATA, NULL, NULL, mime_type, base64_data, filename);
}