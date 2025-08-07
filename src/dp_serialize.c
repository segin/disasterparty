/*
 * dp_serialize.c - Conversation serialization and JSON handling
 * 
 * This module handles serialization and deserialization of conversation
 * messages to/from JSON format, both as strings and files.
 */

#include "dp_private.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int dp_serialize_messages_to_json_str(const dp_message_t* messages, size_t num_messages, char** json_str_out) {
    if (!messages || !json_str_out) return -1;
    
    cJSON* root = cJSON_CreateArray();
    if (!root) return -1;

    for (size_t i = 0; i < num_messages; ++i) {
        const dp_message_t* msg = &messages[i];
        cJSON* msg_obj = cJSON_CreateObject();
        if (!msg_obj) { cJSON_Delete(root); return -1; }

        const char* role_str = "user"; // default
        switch(msg->role) {
            case DP_ROLE_SYSTEM: role_str = "system"; break;
            case DP_ROLE_USER: role_str = "user"; break;
            case DP_ROLE_ASSISTANT: role_str = "assistant"; break;
            case DP_ROLE_TOOL: role_str = "tool"; break;
        }
        cJSON_AddStringToObject(msg_obj, "role", role_str);

        cJSON* parts_array = cJSON_AddArrayToObject(msg_obj, "parts");
        if(!parts_array) { cJSON_Delete(msg_obj); cJSON_Delete(root); return -1; }

        for (size_t j = 0; j < msg->num_parts; ++j) {
            const dp_content_part_t* part = &msg->parts[j];
            cJSON* part_obj = cJSON_CreateObject();
            if(!part_obj) { cJSON_Delete(root); return -1; }

            switch(part->type) {
                case DP_CONTENT_PART_TEXT:
                    cJSON_AddStringToObject(part_obj, "type", "text");
                    cJSON_AddStringToObject(part_obj, "text", part->text);
                    break;
                case DP_CONTENT_PART_IMAGE_URL:
                    cJSON_AddStringToObject(part_obj, "type", "image_url");
                    cJSON_AddStringToObject(part_obj, "url", part->image_url);
                    break;
                case DP_CONTENT_PART_IMAGE_BASE64:
                    cJSON_AddStringToObject(part_obj, "type", "image_base64");
                    cJSON_AddStringToObject(part_obj, "mime_type", part->image_base64.mime_type);
                    cJSON_AddStringToObject(part_obj, "data", part->image_base64.data);
                    break;
                case DP_CONTENT_PART_FILE_DATA:
                    cJSON_AddStringToObject(part_obj, "type", "file_data");
                    cJSON_AddStringToObject(part_obj, "mime_type", part->file_data.mime_type);
                    cJSON_AddStringToObject(part_obj, "data", part->file_data.data);
                    if (part->file_data.filename) {
                        cJSON_AddStringToObject(part_obj, "filename", part->file_data.filename);
                    }
                    break;
                case DP_CONTENT_PART_FILE_REFERENCE:
                    cJSON_AddStringToObject(part_obj, "type", "file_reference");
                    cJSON_AddStringToObject(part_obj, "file_id", part->file_reference.file_id);
                    cJSON_AddStringToObject(part_obj, "mime_type", part->file_reference.mime_type);
                    break;
            }
            cJSON_AddItemToArray(parts_array, part_obj);
        }
        cJSON_AddItemToArray(root, msg_obj);
    }
    
    *json_str_out = cJSON_Print(root); 
    cJSON_Delete(root);

    return (*json_str_out) ? 0 : -1;
}

int dp_deserialize_messages_from_json_str(const char* json_str, dp_message_t** messages_out, size_t* num_messages_out) {
    if (!json_str || !messages_out || !num_messages_out) return -1;

    cJSON* root = cJSON_Parse(json_str);
    if (!root) return -1;

    if (!cJSON_IsArray(root)) {
        cJSON_Delete(root);
        return -1;
    }

    size_t count = cJSON_GetArraySize(root);
    dp_message_t* msg_array = calloc(count, sizeof(dp_message_t));
    if (!msg_array) { cJSON_Delete(root); return -1; }

    int current_msg_idx = 0;
    cJSON* msg_obj = NULL;
    cJSON_ArrayForEach(msg_obj, root) {
        dp_message_t* current_msg = &msg_array[current_msg_idx];
        
        cJSON* role_item = cJSON_GetObjectItemCaseSensitive(msg_obj, "role");
        if (cJSON_IsString(role_item)) {
            if (strcmp(role_item->valuestring, "system") == 0) current_msg->role = DP_ROLE_SYSTEM;
            else if (strcmp(role_item->valuestring, "assistant") == 0) current_msg->role = DP_ROLE_ASSISTANT;
            else if (strcmp(role_item->valuestring, "tool") == 0) current_msg->role = DP_ROLE_TOOL;
            else current_msg->role = DP_ROLE_USER; // Default
        }

        cJSON* parts_array = cJSON_GetObjectItemCaseSensitive(msg_obj, "parts");
        if (cJSON_IsArray(parts_array)) {
            cJSON* part_obj = NULL;
            cJSON_ArrayForEach(part_obj, parts_array) {
                cJSON* type_item = cJSON_GetObjectItemCaseSensitive(part_obj, "type");
                if (cJSON_IsString(type_item)) {
                    if (strcmp(type_item->valuestring, "text") == 0) {
                        cJSON* text_item = cJSON_GetObjectItemCaseSensitive(part_obj, "text");
                        if (cJSON_IsString(text_item)) dp_message_add_text_part(current_msg, text_item->valuestring);
                    } else if (strcmp(type_item->valuestring, "image_url") == 0) {
                        cJSON* url_item = cJSON_GetObjectItemCaseSensitive(part_obj, "url");
                        if (cJSON_IsString(url_item)) dp_message_add_image_url_part(current_msg, url_item->valuestring);
                    } else if (strcmp(type_item->valuestring, "image_base64") == 0) {
                        cJSON* mime_item = cJSON_GetObjectItemCaseSensitive(part_obj, "mime_type");
                        cJSON* data_item = cJSON_GetObjectItemCaseSensitive(part_obj, "data");
                        if (cJSON_IsString(mime_item) && cJSON_IsString(data_item)) {
                            dp_message_add_base64_image_part(current_msg, mime_item->valuestring, data_item->valuestring);
                        }
                    } else if (strcmp(type_item->valuestring, "file_data") == 0) {
                        cJSON* mime_item = cJSON_GetObjectItemCaseSensitive(part_obj, "mime_type");
                        cJSON* data_item = cJSON_GetObjectItemCaseSensitive(part_obj, "data");
                        cJSON* filename_item = cJSON_GetObjectItemCaseSensitive(part_obj, "filename");
                        if (cJSON_IsString(mime_item) && cJSON_IsString(data_item)) {
                            const char* filename = (cJSON_IsString(filename_item)) ? filename_item->valuestring : NULL;
                            dp_message_add_file_data_part(current_msg, mime_item->valuestring, data_item->valuestring, filename);
                        }
                    } else if (strcmp(type_item->valuestring, "file_reference") == 0) {
                        cJSON* file_id_item = cJSON_GetObjectItemCaseSensitive(part_obj, "file_id");
                        cJSON* mime_item = cJSON_GetObjectItemCaseSensitive(part_obj, "mime_type");
                        if (cJSON_IsString(file_id_item) && cJSON_IsString(mime_item)) {
                            dp_message_add_file_reference_part(current_msg, file_id_item->valuestring, mime_item->valuestring);
                        }
                    }
                }
            }
        }
        current_msg_idx++;
    }

    cJSON_Delete(root);
    *messages_out = msg_array;
    *num_messages_out = count;
    return 0;
}

int dp_serialize_messages_to_file(const dp_message_t* messages, size_t num_messages, const char* path) {
    char* json_str = NULL;
    if (dp_serialize_messages_to_json_str(messages, num_messages, &json_str) != 0) {
        return -1;
    }
    if (!json_str) return -1; 

    FILE* fp = fopen(path, "w");
    if (!fp) {
        free(json_str);
        return -1;
    }

    int result = (fputs(json_str, fp) < 0) ? -1 : 0;
    
    fclose(fp);
    free(json_str);
    return result;
}

int dp_deserialize_messages_from_file(const char* path, dp_message_t** messages_out, size_t* num_messages_out) {
    FILE* fp = fopen(path, "r");
    if (!fp) return -1;
    
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (length < 0) {
        fclose(fp);
        return -1;
    }

    char* buffer = malloc(length + 1);
    if (!buffer) {
        fclose(fp);
        return -1;
    }

    if (fread(buffer, 1, length, fp) != (size_t)length) {
        free(buffer);
        fclose(fp);
        return -1;
    }
    buffer[length] = '\0';
    fclose(fp);

    int result = dp_deserialize_messages_from_json_str(buffer, messages_out, num_messages_out);
    free(buffer);
    return result;
}