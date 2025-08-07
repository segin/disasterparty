#define _GNU_SOURCE 
#include "disasterparty.h"
#include "dp_private.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h> 

char* dpinternal_build_gemini_count_tokens_json_payload_with_cjson(const dp_request_config_t* request_config) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    if (request_config->system_prompt && strlen(request_config->system_prompt) > 0) {
        cJSON* sys_instruction = cJSON_AddObjectToObject(root, "system_instruction");
        if (!sys_instruction) { cJSON_Delete(root); return NULL; }
        cJSON* sys_parts_array = cJSON_AddArrayToObject(sys_instruction, "parts");
        if (!sys_parts_array) { cJSON_Delete(root); return NULL; }
        cJSON* sys_part_obj = cJSON_CreateObject();
        if (!sys_part_obj) { cJSON_Delete(root); return NULL; }
        cJSON_AddStringToObject(sys_part_obj, "text", request_config->system_prompt);
        cJSON_AddItemToArray(sys_parts_array, sys_part_obj);
    }

    cJSON *contents_array = cJSON_AddArrayToObject(root, "contents");
    if (!contents_array) { cJSON_Delete(root); return NULL; }

    for (size_t i = 0; i < request_config->num_messages; ++i) {
        const dp_message_t* msg = &request_config->messages[i];
        if (msg->role == DP_ROLE_SYSTEM) continue;

        cJSON *content_obj = cJSON_CreateObject();
        if (!content_obj) { cJSON_Delete(root); return NULL; }

        const char* role_str = (msg->role == DP_ROLE_ASSISTANT) ? "model" : "user";
        cJSON_AddStringToObject(content_obj, "role", role_str);

        cJSON *parts_array = cJSON_AddArrayToObject(content_obj, "parts");
        if (!parts_array) { cJSON_Delete(root); return NULL; }

        for (size_t j = 0; j < msg->num_parts; ++j) {
            const dp_content_part_t* part = &msg->parts[j];
            cJSON *part_obj = cJSON_CreateObject();
            if (!part_obj) { cJSON_Delete(root); return NULL; }

            if (part->type == DP_CONTENT_PART_TEXT) {
                cJSON_AddStringToObject(part_obj, "text", part->text);
            } else if (part->type == DP_CONTENT_PART_IMAGE_BASE64) {
                cJSON *inline_data_obj = cJSON_CreateObject();
                if (!inline_data_obj) {cJSON_Delete(part_obj); cJSON_Delete(root); return NULL;}
                cJSON_AddStringToObject(inline_data_obj, "mime_type", part->image_base64.mime_type);
                cJSON_AddStringToObject(inline_data_obj, "data", part->image_base64.data); 
                cJSON_AddItemToObject(part_obj, "inline_data", inline_data_obj);
            } else if (part->type == DP_CONTENT_PART_IMAGE_URL) {
                char temp_text[512];
                snprintf(temp_text, sizeof(temp_text), "Image at URL: %s", part->image_url);
                cJSON_AddStringToObject(part_obj, "text", temp_text);
            } else if (part->type == DP_CONTENT_PART_FILE_DATA) {
                // Gemini supports file data via inline_data similar to images
                cJSON *inline_data_obj = cJSON_CreateObject();
                if (!inline_data_obj) {cJSON_Delete(part_obj); cJSON_Delete(root); return NULL;}
                cJSON_AddStringToObject(inline_data_obj, "mime_type", part->file_data.mime_type);
                cJSON_AddStringToObject(inline_data_obj, "data", part->file_data.data);
                cJSON_AddItemToObject(part_obj, "inline_data", inline_data_obj);
            } else if (part->type == DP_CONTENT_PART_FILE_REFERENCE) {
                // Gemini supports file references via file_data
                cJSON *file_data_obj = cJSON_CreateObject();
                if (!file_data_obj) {cJSON_Delete(part_obj); cJSON_Delete(root); return NULL;}
                cJSON_AddStringToObject(file_data_obj, "mime_type", part->file_reference.mime_type);
                cJSON_AddStringToObject(file_data_obj, "file_uri", part->file_reference.file_id);
                cJSON_AddItemToObject(part_obj, "file_data", file_data_obj);
            }
            cJSON_AddItemToArray(parts_array, part_obj);
        }
        cJSON_AddItemToArray(contents_array, content_obj);
    }

    char* json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_string; 
}

char* dpinternal_build_anthropic_count_tokens_json_payload_with_cjson(const dp_request_config_t* request_config) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON_AddStringToObject(root, "model", request_config->model);

    if (request_config->system_prompt && strlen(request_config->system_prompt) > 0) {
        cJSON_AddStringToObject(root, "system", request_config->system_prompt);
    }

    cJSON *messages_array = cJSON_AddArrayToObject(root, "messages");
    if (!messages_array) { cJSON_Delete(root); return NULL; }

    for (size_t i = 0; i < request_config->num_messages; ++i) {
        const dp_message_t* msg = &request_config->messages[i];
        if (msg->role == DP_ROLE_SYSTEM) continue;

        cJSON *msg_obj = cJSON_CreateObject();
        if (!msg_obj) { cJSON_Delete(root); return NULL; }

        const char* role_str = (msg->role == DP_ROLE_ASSISTANT) ? "assistant" : "user";
        cJSON_AddStringToObject(msg_obj, "role", role_str);

        if (msg->num_parts == 1 && msg->parts[0].type == DP_CONTENT_PART_TEXT) {
            cJSON_AddStringToObject(msg_obj, "content", msg->parts[0].text);
        } else {
            cJSON *content_array_for_anthropic = cJSON_CreateArray();
            if(!content_array_for_anthropic) { cJSON_Delete(msg_obj); cJSON_Delete(root); return NULL; }

            for (size_t j = 0; j < msg->num_parts; ++j) {
                const dp_content_part_t* part = &msg->parts[j];
                cJSON *part_obj = cJSON_CreateObject();
                if (!part_obj) { cJSON_Delete(content_array_for_anthropic) ;cJSON_Delete(msg_obj); cJSON_Delete(root); return NULL; }

                if (part->type == DP_CONTENT_PART_TEXT) {
                    cJSON_AddStringToObject(part_obj, "type", "text");
                    cJSON_AddStringToObject(part_obj, "text", part->text);
                } else if (part->type == DP_CONTENT_PART_IMAGE_BASE64) {
                    cJSON_AddStringToObject(part_obj, "type", "image");
                    cJSON *source_obj = cJSON_CreateObject();
                    if(!source_obj) {cJSON_Delete(part_obj); cJSON_Delete(content_array_for_anthropic); cJSON_Delete(msg_obj); cJSON_Delete(root); return NULL;}
                    cJSON_AddStringToObject(source_obj, "type", "base64");
                    cJSON_AddStringToObject(source_obj, "media_type", part->image_base64.mime_type);
                    cJSON_AddStringToObject(source_obj, "data", part->image_base64.data);
                    cJSON_AddItemToObject(part_obj, "source", source_obj);
                } else if (part->type == DP_CONTENT_PART_IMAGE_URL) {
                    char temp_text[512];
                    snprintf(temp_text, sizeof(temp_text), "Image referenced by URL: %s (Anthropic prefers direct image data)", part->image_url);
                    cJSON_AddStringToObject(part_obj, "type", "text");
                    cJSON_AddStringToObject(part_obj, "text", temp_text);
                } else if (part->type == DP_CONTENT_PART_FILE_DATA) {
                    // Anthropic supports file data similar to images with base64 encoding
                    cJSON_AddStringToObject(part_obj, "type", "document");
                    cJSON *source_obj = cJSON_CreateObject();
                    if(!source_obj) {cJSON_Delete(part_obj); cJSON_Delete(content_array_for_anthropic); cJSON_Delete(msg_obj); cJSON_Delete(root); return NULL;}
                    cJSON_AddStringToObject(source_obj, "type", "base64");
                    cJSON_AddStringToObject(source_obj, "media_type", part->file_data.mime_type);
                    cJSON_AddStringToObject(source_obj, "data", part->file_data.data);
                    cJSON_AddItemToObject(part_obj, "source", source_obj);
                } else if (part->type == DP_CONTENT_PART_FILE_REFERENCE) {
                    // Anthropic doesn't support file references directly, convert to text
                    char temp_text[512];
                    snprintf(temp_text, sizeof(temp_text), "File reference: %s (type: %s) - Anthropic requires direct file data", 
                             part->file_reference.file_id, part->file_reference.mime_type);
                    cJSON_AddStringToObject(part_obj, "type", "text");
                    cJSON_AddStringToObject(part_obj, "text", temp_text);
                }
                cJSON_AddItemToArray(content_array_for_anthropic, part_obj);
            }
            cJSON_AddItemToObject(msg_obj, "content", content_array_for_anthropic);
        }
        cJSON_AddItemToArray(messages_array, msg_obj);
    }
    
    char* json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_string;
}


char* dpinternal_build_openai_json_payload_with_cjson(const dp_request_config_t* request_config, const dp_context_t* context) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON_AddStringToObject(root, "model", request_config->model);
    if (request_config->temperature >= 0.0) cJSON_AddNumberToObject(root, "temperature", request_config->temperature);
    if (request_config->max_tokens > 0) {
        const char* token_param = (context->token_param_preference == DP_TOKEN_PARAM_MAX_COMPLETION_TOKENS) 
                                  ? "max_completion_tokens" : "max_tokens";
        cJSON_AddNumberToObject(root, token_param, request_config->max_tokens);
    }
    if (request_config->stream) cJSON_AddTrueToObject(root, "stream");
    if (request_config->top_p > 0.0) cJSON_AddNumberToObject(root, "top_p", request_config->top_p);
    
    if (request_config->stop_sequences && request_config->num_stop_sequences > 0) {
        // OpenAI can take a string or an array of strings. We'll provide an array.
        cJSON* stop_array = cJSON_CreateStringArray(request_config->stop_sequences, request_config->num_stop_sequences);
        cJSON_AddItemToObject(root, "stop", stop_array);
    }

    cJSON *messages_array = cJSON_AddArrayToObject(root, "messages");
    if (!messages_array) {
        cJSON_Delete(root);
        return NULL;
    }

    if (request_config->system_prompt && strlen(request_config->system_prompt) > 0) {
        cJSON* sys_msg_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(sys_msg_obj, "role", "system");
        cJSON_AddStringToObject(sys_msg_obj, "content", request_config->system_prompt);
        cJSON_AddItemToArray(messages_array, sys_msg_obj);
    }

    for (size_t i = 0; i < request_config->num_messages; ++i) {
        const dp_message_t* msg = &request_config->messages[i];
        if (msg->role == DP_ROLE_SYSTEM) continue;

        cJSON *msg_obj = cJSON_CreateObject();
        if (!msg_obj) { cJSON_Delete(root); return NULL; }

        const char* role_str = NULL;
        switch (msg->role) {
            case DP_ROLE_USER: role_str = "user"; break;
            case DP_ROLE_ASSISTANT: role_str = "assistant"; break;
            case DP_ROLE_TOOL: role_str = "tool"; break;
            default: role_str = "user";
        }
        cJSON_AddStringToObject(msg_obj, "role", role_str);

        if (msg->num_parts == 1 && msg->parts[0].type == DP_CONTENT_PART_TEXT) {
            cJSON_AddStringToObject(msg_obj, "content", msg->parts[0].text);
        } else {
            cJSON *content_array = cJSON_AddArrayToObject(msg_obj, "content");
            if (!content_array) { cJSON_Delete(root); return NULL; }
            for (size_t j = 0; j < msg->num_parts; ++j) {
                const dp_content_part_t* part = &msg->parts[j];
                cJSON *part_obj = cJSON_CreateObject();
                if (!part_obj) { cJSON_Delete(root); return NULL; }
                if (part->type == DP_CONTENT_PART_TEXT) {
                    cJSON_AddStringToObject(part_obj, "type", "text");
                    cJSON_AddStringToObject(part_obj, "text", part->text);
                } else if (part->type == DP_CONTENT_PART_IMAGE_URL) {
                    cJSON_AddStringToObject(part_obj, "type", "image_url");
                    cJSON *img_url_obj = cJSON_CreateObject();
                    if (!img_url_obj) { cJSON_Delete(part_obj); cJSON_Delete(root); return NULL;}
                    cJSON_AddStringToObject(img_url_obj, "url", part->image_url);
                    cJSON_AddItemToObject(part_obj, "image_url", img_url_obj);
                }
                else if (part->type == DP_CONTENT_PART_IMAGE_BASE64) {
                    cJSON_AddStringToObject(part_obj, "type", "image_url"); 
                    cJSON *img_url_obj = cJSON_CreateObject();
                    if (!img_url_obj) { cJSON_Delete(part_obj); cJSON_Delete(root); return NULL;}
                    char* data_uri;
                    size_t data_uri_len = strlen("data:") + strlen(part->image_base64.mime_type) + strlen(";base64,") + strlen(part->image_base64.data) + 1;
                    data_uri = malloc(data_uri_len);
                    if (data_uri) {
                        snprintf(data_uri, data_uri_len, "data:%s;base64,%s", part->image_base64.mime_type, part->image_base64.data);
                        cJSON_AddStringToObject(img_url_obj, "url", data_uri);
                        free(data_uri);
                    } else {
                        cJSON_Delete(img_url_obj); 
                        cJSON_Delete(part_obj);    
                        cJSON_Delete(root);        
                        return NULL;               
                    }
                    cJSON_AddItemToObject(part_obj, "image_url", img_url_obj);
                }
                else if (part->type == DP_CONTENT_PART_FILE_DATA) {
                    // OpenAI doesn't have native file attachment support, so we'll use a text representation
                    cJSON_AddStringToObject(part_obj, "type", "text");
                    char* file_text = NULL;
                    if (part->file_data.filename) {
                        if (dpinternal_safe_asprintf(&file_text, "[File: %s (%s)]\nBase64 Data: %s", 
                                part->file_data.filename, part->file_data.mime_type, part->file_data.data) == -1) {
                            cJSON_Delete(part_obj);
                            cJSON_Delete(root);
                            return NULL;
                        }
                    } else {
                        if (dpinternal_safe_asprintf(&file_text, "[File (%s)]\nBase64 Data: %s", 
                                part->file_data.mime_type, part->file_data.data) == -1) {
                            cJSON_Delete(part_obj);
                            cJSON_Delete(root);
                            return NULL;
                        }
                    }
                    if (file_text) {
                        cJSON_AddStringToObject(part_obj, "text", file_text);
                        free(file_text);
                    } else {
                        cJSON_Delete(part_obj);
                        cJSON_Delete(root);
                        return NULL;
                    }
                } else if (part->type == DP_CONTENT_PART_FILE_REFERENCE) {
                    // OpenAI doesn't support file references directly, convert to text
                    cJSON_AddStringToObject(part_obj, "type", "text");
                    char* file_ref_text = NULL;
                    if (dpinternal_safe_asprintf(&file_ref_text, "[File Reference: %s (type: %s)] - OpenAI requires direct file data", 
                            part->file_reference.file_id, part->file_reference.mime_type) == -1) {
                        cJSON_Delete(part_obj);
                        cJSON_Delete(root);
                        return NULL;
                    }
                    if (file_ref_text) {
                        cJSON_AddStringToObject(part_obj, "text", file_ref_text);
                        free(file_ref_text);
                    } else {
                        cJSON_Delete(part_obj);
                        cJSON_Delete(root);
                        return NULL;
                    }
                }
                cJSON_AddItemToArray(content_array, part_obj);
            }
        }
        cJSON_AddItemToArray(messages_array, msg_obj);
    }

    char* json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_string; 
}

char* dpinternal_build_gemini_json_payload_with_cjson(const dp_request_config_t* request_config) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    if (request_config->system_prompt && strlen(request_config->system_prompt) > 0) {
        cJSON* sys_instruction = cJSON_AddObjectToObject(root, "system_instruction");
        if (!sys_instruction) { cJSON_Delete(root); return NULL; }
        cJSON* sys_parts_array = cJSON_AddArrayToObject(sys_instruction, "parts");
        if (!sys_parts_array) { cJSON_Delete(root); return NULL; }
        cJSON* sys_part_obj = cJSON_CreateObject();
        if (!sys_part_obj) { cJSON_Delete(root); return NULL; }
        cJSON_AddStringToObject(sys_part_obj, "text", request_config->system_prompt);
        cJSON_AddItemToArray(sys_parts_array, sys_part_obj);
    }

    cJSON *contents_array = cJSON_AddArrayToObject(root, "contents");
    if (!contents_array) { cJSON_Delete(root); return NULL; }

    for (size_t i = 0; i < request_config->num_messages; ++i) {
        const dp_message_t* msg = &request_config->messages[i];
        if (msg->role == DP_ROLE_SYSTEM) continue;

        cJSON *content_obj = cJSON_CreateObject();
        if (!content_obj) { cJSON_Delete(root); return NULL; }

        const char* role_str = (msg->role == DP_ROLE_ASSISTANT) ? "model" : "user";
        cJSON_AddStringToObject(content_obj, "role", role_str);

        cJSON *parts_array = cJSON_AddArrayToObject(content_obj, "parts");
        if (!parts_array) { cJSON_Delete(root); return NULL; }

        for (size_t j = 0; j < msg->num_parts; ++j) {
            const dp_content_part_t* part = &msg->parts[j];
            cJSON *part_obj = cJSON_CreateObject();
            if (!part_obj) { cJSON_Delete(root); return NULL; }

            if (part->type == DP_CONTENT_PART_TEXT) {
                cJSON_AddStringToObject(part_obj, "text", part->text);
            } else if (part->type == DP_CONTENT_PART_IMAGE_BASE64) {
                cJSON *inline_data_obj = cJSON_CreateObject();
                if (!inline_data_obj) {cJSON_Delete(part_obj); cJSON_Delete(root); return NULL;}
                cJSON_AddStringToObject(inline_data_obj, "mime_type", part->image_base64.mime_type);
                cJSON_AddStringToObject(inline_data_obj, "data", part->image_base64.data); 
                cJSON_AddItemToObject(part_obj, "inline_data", inline_data_obj);
            } else if (part->type == DP_CONTENT_PART_IMAGE_URL) {
                char temp_text[512];
                snprintf(temp_text, sizeof(temp_text), "Image at URL: %s", part->image_url);
                cJSON_AddStringToObject(part_obj, "text", temp_text);
            } else if (part->type == DP_CONTENT_PART_FILE_DATA) {
                // Gemini supports file data via inline_data similar to images
                cJSON *inline_data_obj = cJSON_CreateObject();
                if (!inline_data_obj) {cJSON_Delete(part_obj); cJSON_Delete(root); return NULL;}
                cJSON_AddStringToObject(inline_data_obj, "mime_type", part->file_data.mime_type);
                cJSON_AddStringToObject(inline_data_obj, "data", part->file_data.data);
                cJSON_AddItemToObject(part_obj, "inline_data", inline_data_obj);
            } else if (part->type == DP_CONTENT_PART_FILE_REFERENCE) {
                // Gemini supports file references via file_data
                cJSON *file_data_obj = cJSON_CreateObject();
                if (!file_data_obj) {cJSON_Delete(part_obj); cJSON_Delete(root); return NULL;}
                cJSON_AddStringToObject(file_data_obj, "mime_type", part->file_reference.mime_type);
                cJSON_AddStringToObject(file_data_obj, "file_uri", part->file_reference.file_id);
                cJSON_AddItemToObject(part_obj, "file_data", file_data_obj);
            }
            cJSON_AddItemToArray(parts_array, part_obj);
        }
        cJSON_AddItemToArray(contents_array, content_obj);
    }

    cJSON *gen_config = cJSON_AddObjectToObject(root, "generationConfig");
    if (!gen_config) { cJSON_Delete(root); return NULL; }
    if (request_config->temperature >= 0.0) cJSON_AddNumberToObject(gen_config, "temperature", request_config->temperature);
    if (request_config->max_tokens > 0) cJSON_AddNumberToObject(gen_config, "maxOutputTokens", request_config->max_tokens);
    if (request_config->top_p > 0.0) cJSON_AddNumberToObject(gen_config, "topP", request_config->top_p);
    if (request_config->top_k > 0) cJSON_AddNumberToObject(gen_config, "topK", request_config->top_k);
    if (request_config->stop_sequences && request_config->num_stop_sequences > 0) {
        cJSON* stop_array = cJSON_CreateStringArray(request_config->stop_sequences, request_config->num_stop_sequences);
        cJSON_AddItemToObject(gen_config, "stopSequences", stop_array);
    }


    char* json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_string; 
}

char* dpinternal_build_anthropic_json_payload_with_cjson(const dp_request_config_t* request_config) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON_AddStringToObject(root, "model", request_config->model);
    cJSON_AddNumberToObject(root, "max_tokens", request_config->max_tokens > 0 ? request_config->max_tokens : 4096);
    if (request_config->temperature >= 0.0 && request_config->temperature <= 1.0) { 
        cJSON_AddNumberToObject(root, "temperature", request_config->temperature);
    }
    if (request_config->top_p > 0.0) {
        cJSON_AddNumberToObject(root, "top_p", request_config->top_p);
    }
    if (request_config->top_k > 0) {
        cJSON_AddNumberToObject(root, "top_k", request_config->top_k);
    }
    if (request_config->stop_sequences && request_config->num_stop_sequences > 0) {
        cJSON* stop_array = cJSON_CreateStringArray(request_config->stop_sequences, request_config->num_stop_sequences);
        cJSON_AddItemToObject(root, "stop_sequences", stop_array);
    }

    if (request_config->system_prompt && strlen(request_config->system_prompt) > 0) {
        cJSON_AddStringToObject(root, "system", request_config->system_prompt);
    }

    cJSON *messages_array = cJSON_AddArrayToObject(root, "messages");
    if (!messages_array) { cJSON_Delete(root); return NULL; }

    for (size_t i = 0; i < request_config->num_messages; ++i) {
        const dp_message_t* msg = &request_config->messages[i];
        if (msg->role == DP_ROLE_SYSTEM) continue;

        cJSON *msg_obj = cJSON_CreateObject();
        if (!msg_obj) { cJSON_Delete(root); return NULL; }

        const char* role_str = (msg->role == DP_ROLE_ASSISTANT) ? "assistant" : "user";
        cJSON_AddStringToObject(msg_obj, "role", role_str);

        cJSON *content_array_for_anthropic = cJSON_CreateArray();
        if(!content_array_for_anthropic) { cJSON_Delete(msg_obj); cJSON_Delete(root); return NULL; }

        for (size_t j = 0; j < msg->num_parts; ++j) {
            const dp_content_part_t* part = &msg->parts[j];
            cJSON *part_obj = cJSON_CreateObject();
            if (!part_obj) { cJSON_Delete(content_array_for_anthropic) ;cJSON_Delete(msg_obj); cJSON_Delete(root); return NULL; }

            if (part->type == DP_CONTENT_PART_TEXT) {
                cJSON_AddStringToObject(part_obj, "type", "text");
                cJSON_AddStringToObject(part_obj, "text", part->text);
            } else if (part->type == DP_CONTENT_PART_IMAGE_BASE64) {
                cJSON_AddStringToObject(part_obj, "type", "image");
                cJSON *source_obj = cJSON_CreateObject();
                if(!source_obj) {cJSON_Delete(part_obj); cJSON_Delete(content_array_for_anthropic); cJSON_Delete(msg_obj); cJSON_Delete(root); return NULL;}
                cJSON_AddStringToObject(source_obj, "type", "base64");
                cJSON_AddStringToObject(source_obj, "media_type", part->image_base64.mime_type);
                cJSON_AddStringToObject(source_obj, "data", part->image_base64.data);
                cJSON_AddItemToObject(part_obj, "source", source_obj);
            } else if (part->type == DP_CONTENT_PART_IMAGE_URL) {
                char temp_text[512];
                snprintf(temp_text, sizeof(temp_text), "Image referenced by URL: %s (Anthropic prefers direct image data)", part->image_url);
                cJSON_AddStringToObject(part_obj, "type", "text");
                cJSON_AddStringToObject(part_obj, "text", temp_text);
            } else if (part->type == DP_CONTENT_PART_FILE_DATA) {
                // Anthropic supports file data similar to images with base64 encoding
                cJSON_AddStringToObject(part_obj, "type", "document");
                cJSON *source_obj = cJSON_CreateObject();
                if(!source_obj) {cJSON_Delete(part_obj); cJSON_Delete(content_array_for_anthropic); cJSON_Delete(msg_obj); cJSON_Delete(root); return NULL;}
                cJSON_AddStringToObject(source_obj, "type", "base64");
                cJSON_AddStringToObject(source_obj, "media_type", part->file_data.mime_type);
                cJSON_AddStringToObject(source_obj, "data", part->file_data.data);
                cJSON_AddItemToObject(part_obj, "source", source_obj);
            } else if (part->type == DP_CONTENT_PART_FILE_REFERENCE) {
                // Anthropic doesn't support file references directly, convert to text
                char temp_text[512];
                snprintf(temp_text, sizeof(temp_text), "File reference: %s (type: %s) - Anthropic requires direct file data", 
                         part->file_reference.file_id, part->file_reference.mime_type);
                cJSON_AddStringToObject(part_obj, "type", "text");
                cJSON_AddStringToObject(part_obj, "text", temp_text);
            }
            cJSON_AddItemToArray(content_array_for_anthropic, part_obj);
        }
        cJSON_AddItemToObject(msg_obj, "content", content_array_for_anthropic);
        cJSON_AddItemToArray(messages_array, msg_obj);
    }
    
    if (request_config->stream) {
        cJSON_AddTrueToObject(root, "stream");
    }

    char* json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_string;
}

// Helper function to detect if an error is related to unrecognized token parameters
bool dpinternal_is_token_parameter_error(const char* error_response, long http_status) {
    if (http_status != 400 || !error_response) {
        return false;
    }
    
    // Check for common error patterns indicating unrecognized max_completion_tokens parameter
    return (strstr(error_response, "max_completion_tokens") != NULL &&
            (strstr(error_response, "Unrecognized request argument") != NULL ||
             strstr(error_response, "unrecognized") != NULL ||
             strstr(error_response, "invalid") != NULL ||
             strstr(error_response, "unknown") != NULL));
}

// Helper function to perform OpenAI request with automatic token parameter fallback
CURLcode dpinternal_perform_openai_request_with_fallback(CURL* curl, dp_context_t* context, 
                                                                const dp_request_config_t* request_config,
                                                                memory_struct_t* chunk_mem,
                                                                long* http_status_code) {
    char* json_payload_str = dpinternal_build_openai_json_payload_with_cjson(request_config, context);
    if (!json_payload_str) {
        return CURLE_OUT_OF_MEMORY;
    }
    
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload_str);
    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_status_code);
    
    // Check if we need to retry with fallback parameter
    if (res == CURLE_OK && *http_status_code == 400 && 
        context->token_param_preference == DP_TOKEN_PARAM_MAX_COMPLETION_TOKENS &&
        dpinternal_is_token_parameter_error(chunk_mem->memory, *http_status_code)) {
        
        // Switch to legacy parameter and retry
        context->token_param_preference = DP_TOKEN_PARAM_MAX_TOKENS;
        free(json_payload_str);
        
        // Reset response buffer for retry
        if (chunk_mem->memory) {
            free(chunk_mem->memory);
            chunk_mem->memory = NULL;
            chunk_mem->size = 0;
        }
        
        // Build new payload with legacy parameter
        json_payload_str = dpinternal_build_openai_json_payload_with_cjson(request_config, context);
        if (!json_payload_str) {
            return CURLE_OUT_OF_MEMORY;
        }
        
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload_str);
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_status_code);
    }
    
    free(json_payload_str);
    return res;
}



char* dpinternal_extract_text_from_full_response_with_cjson(const char* json_response_str, dp_provider_type_t provider, char** finish_reason_out) {
    if (finish_reason_out) *finish_reason_out = NULL;
    if (!json_response_str) return NULL;

    cJSON *root = cJSON_Parse(json_response_str);
    if (!root) {
        return NULL;
    }

    char* extracted_text = NULL;
    cJSON *item_array = NULL; 

    if (provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
        item_array = cJSON_GetObjectItemCaseSensitive(root, "choices");
        if (cJSON_IsArray(item_array) && cJSON_GetArraySize(item_array) > 0) {
            cJSON *first_choice = cJSON_GetArrayItem(item_array, 0);
            if (first_choice) {
                cJSON *message = cJSON_GetObjectItemCaseSensitive(first_choice, "message");
                if (message) {
                    cJSON *content = cJSON_GetObjectItemCaseSensitive(message, "content");
                    if (cJSON_IsString(content) && content->valuestring) {
                        extracted_text = dpinternal_strdup(content->valuestring);
                    }
                }
                if (finish_reason_out) {
                    cJSON *reason_item = cJSON_GetObjectItemCaseSensitive(first_choice, "finish_reason");
                    if (cJSON_IsString(reason_item) && reason_item->valuestring) {
                        *finish_reason_out = dpinternal_strdup(reason_item->valuestring);
                    }
                }
            }
        }
    } else if (provider == DP_PROVIDER_GOOGLE_GEMINI) {
        item_array = cJSON_GetObjectItemCaseSensitive(root, "candidates");
        if (cJSON_IsArray(item_array) && cJSON_GetArraySize(item_array) > 0) {
            cJSON *first_candidate = cJSON_GetArrayItem(item_array, 0);
            if (first_candidate) {
                cJSON *content = cJSON_GetObjectItemCaseSensitive(first_candidate, "content");
                if (content) {
                    cJSON *parts_array = cJSON_GetObjectItemCaseSensitive(content, "parts");
                    if (cJSON_IsArray(parts_array) && cJSON_GetArraySize(parts_array) > 0) {
                        cJSON* part_item = NULL;
                        cJSON_ArrayForEach(part_item, parts_array) {
                            cJSON *text_item = cJSON_GetObjectItemCaseSensitive(part_item, "text");
                            if (cJSON_IsString(text_item) && text_item->valuestring) {
                                extracted_text = dpinternal_strdup(text_item->valuestring);
                                break; 
                            }
                        }
                    }
                }
                if (finish_reason_out) { 
                     cJSON *reason_item = cJSON_GetObjectItemCaseSensitive(first_candidate, "finishReason"); 
                     if (cJSON_IsString(reason_item) && reason_item->valuestring) {
                         *finish_reason_out = dpinternal_strdup(reason_item->valuestring);
                     }
                }
            }
        }
        if (finish_reason_out && !*finish_reason_out) { 
            cJSON *prompt_feedback = cJSON_GetObjectItemCaseSensitive(root, "promptFeedback");
            if (prompt_feedback) {
                cJSON *reason_item = cJSON_GetObjectItemCaseSensitive(prompt_feedback, "finishReason");
                if (cJSON_IsString(reason_item) && reason_item->valuestring) {
                    *finish_reason_out = dpinternal_strdup(reason_item->valuestring);
                }
            }
        }
    } else if (provider == DP_PROVIDER_ANTHROPIC) {
        item_array = cJSON_GetObjectItemCaseSensitive(root, "content");
        if(cJSON_IsArray(item_array) && cJSON_GetArraySize(item_array) > 0) {
            cJSON* content_block = cJSON_GetArrayItem(item_array, 0); 
            if(content_block && cJSON_IsObject(content_block)) {
                cJSON* text_item = cJSON_GetObjectItemCaseSensitive(content_block, "text");
                if(cJSON_IsString(text_item) && text_item->valuestring) {
                    extracted_text = dpinternal_strdup(text_item->valuestring);
                }
            }
        }
        if(finish_reason_out) {
            cJSON* reason_item = cJSON_GetObjectItemCaseSensitive(root, "stop_reason");
            if(cJSON_IsString(reason_item) && reason_item->valuestring) {
                *finish_reason_out = dpinternal_strdup(reason_item->valuestring);
            }
        }
    }
    
    cJSON_Delete(root);
    return extracted_text;
}










void dp_free_response_content(dp_response_t* response) {
    if (!response) return;
    if (response->parts) {
        for (size_t i = 0; i < response->num_parts; ++i) {
            free(response->parts[i].text); 
        }
        free(response->parts);
    }
    free(response->error_message);
    free(response->finish_reason);
    memset(response, 0, sizeof(dp_response_t)); 
}






