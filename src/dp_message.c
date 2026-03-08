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
                } else if (part->type == DP_CONTENT_PART_FILE_REFERENCE) {
                    free(part->file_reference.file_id);
                    free(part->file_reference.mime_type);
                } else if (part->type == DP_CONTENT_PART_TOOL_CALL) {
                    free(part->tool_call.id);
                    free(part->tool_call.function_name);
                    free(part->tool_call.arguments_json);
                } else if (part->type == DP_CONTENT_PART_TOOL_RESULT) {
                    free(part->tool_result.tool_call_id);
                    free(part->tool_result.content);
                } else if (part->type == DP_CONTENT_PART_THINKING) {
                    free(part->thinking.thinking);
                    free(part->thinking.signature);
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
                                   const char* filename_content, const char* file_id_content) {
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
    } else if (type == DP_CONTENT_PART_FILE_REFERENCE && file_id_content && mime_type_content) {
        new_part->file_reference.file_id = dpinternal_strdup(file_id_content);
        new_part->file_reference.mime_type = dpinternal_strdup(mime_type_content);
        success = (new_part->file_reference.file_id != NULL && new_part->file_reference.mime_type != NULL);
        if (!success) {
            free(new_part->file_reference.file_id); new_part->file_reference.file_id = NULL;
            free(new_part->file_reference.mime_type); new_part->file_reference.mime_type = NULL;
        }
    }
    if (success) message->num_parts++;
    else fprintf(stderr, "Failed to allocate memory for Disaster Party message content part.\n");
    return success;
}

bool dp_message_add_text_part(dp_message_t* message, const char* text) {
    return dpinternal_message_add_part_internal(message, DP_CONTENT_PART_TEXT, text, NULL, NULL, NULL, NULL, NULL);
}

bool dp_message_add_image_url_part(dp_message_t* message, const char* image_url) {
    return dpinternal_message_add_part_internal(message, DP_CONTENT_PART_IMAGE_URL, NULL, image_url, NULL, NULL, NULL, NULL);
}

bool dp_message_add_base64_image_part(dp_message_t* message, const char* mime_type, const char* base64_data) {
    return dpinternal_message_add_part_internal(message, DP_CONTENT_PART_IMAGE_BASE64, NULL, NULL, mime_type, base64_data, NULL, NULL);
}

bool dp_message_add_file_data_part(dp_message_t* message, const char* mime_type, const char* base64_data, const char* filename) {
    return dpinternal_message_add_part_internal(message, DP_CONTENT_PART_FILE_DATA, NULL, NULL, mime_type, base64_data, filename, NULL);
}

bool dp_message_add_file_reference_part(dp_message_t* message, const char* file_id, const char* mime_type) {
    return dpinternal_message_add_part_internal(message, DP_CONTENT_PART_FILE_REFERENCE, NULL, NULL, mime_type, NULL, NULL, file_id);
}

bool dp_message_add_tool_call_part(dp_message_t* message, const char* id, const char* function_name, const char* arguments_json) {
    if (!message) return false;
    dp_content_part_t* new_parts_array = realloc(message->parts, (message->num_parts + 1) * sizeof(dp_content_part_t));
    if (!new_parts_array) return false;
    message->parts = new_parts_array;

    dp_content_part_t* new_part = &message->parts[message->num_parts];
    memset(new_part, 0, sizeof(dp_content_part_t));
    new_part->type = DP_CONTENT_PART_TOOL_CALL;

    bool success = true;
    if (id) {
        new_part->tool_call.id = dpinternal_strdup(id);
        if (!new_part->tool_call.id) success = false;
    }
    if (function_name && success) {
        new_part->tool_call.function_name = dpinternal_strdup(function_name);
        if (!new_part->tool_call.function_name) success = false;
    }
    if (arguments_json && success) {
        new_part->tool_call.arguments_json = dpinternal_strdup(arguments_json);
        if (!new_part->tool_call.arguments_json) success = false;
    }

    if (success) {
        message->num_parts++;
    } else {
        free(new_part->tool_call.id); new_part->tool_call.id = NULL;
        free(new_part->tool_call.function_name); new_part->tool_call.function_name = NULL;
        free(new_part->tool_call.arguments_json); new_part->tool_call.arguments_json = NULL;
        fprintf(stderr, "Failed to allocate memory for Disaster Party tool call part.\n");
    }
    return success;
}

bool dp_message_add_tool_result_part(dp_message_t* message, const char* tool_call_id, const char* content, bool is_error) {
    if (!message) return false;
    dp_content_part_t* new_parts_array = realloc(message->parts, (message->num_parts + 1) * sizeof(dp_content_part_t));
    if (!new_parts_array) return false;
    message->parts = new_parts_array;

    dp_content_part_t* new_part = &message->parts[message->num_parts];
    memset(new_part, 0, sizeof(dp_content_part_t));
    new_part->type = DP_CONTENT_PART_TOOL_RESULT;
    new_part->tool_result.is_error = is_error;

    bool success = true;
    if (tool_call_id) {
        new_part->tool_result.tool_call_id = dpinternal_strdup(tool_call_id);
        if (!new_part->tool_result.tool_call_id) success = false;
    }
    if (content && success) {
        new_part->tool_result.content = dpinternal_strdup(content);
        if (!new_part->tool_result.content) success = false;
    }

    if (success) {
        message->num_parts++;
    } else {
        free(new_part->tool_result.tool_call_id); new_part->tool_result.tool_call_id = NULL;
        free(new_part->tool_result.content); new_part->tool_result.content = NULL;
        fprintf(stderr, "Failed to allocate memory for Disaster Party tool result part.\n");
    }
    return success;
}

bool dp_message_add_thinking_part(dp_message_t* message, const char* thinking, const char* signature) {
    if (!message) return false;
    dp_content_part_t* new_parts_array = realloc(message->parts, (message->num_parts + 1) * sizeof(dp_content_part_t));
    if (!new_parts_array) return false;
    message->parts = new_parts_array;

    dp_content_part_t* new_part = &message->parts[message->num_parts];
    memset(new_part, 0, sizeof(dp_content_part_t));
    new_part->type = DP_CONTENT_PART_THINKING;

    bool success = true;
    if (thinking) {
        new_part->thinking.thinking = dpinternal_strdup(thinking);
        if (!new_part->thinking.thinking) success = false;
    }
    if (signature && success) {
        new_part->thinking.signature = dpinternal_strdup(signature);
        if (!new_part->thinking.signature) success = false;
    }

    if (success) {
        message->num_parts++;
    } else {
        free(new_part->thinking.thinking); new_part->thinking.thinking = NULL;
        free(new_part->thinking.signature); new_part->thinking.signature = NULL;
        fprintf(stderr, "Failed to allocate memory for Disaster Party thinking part.\n");
    }
    return success;
}