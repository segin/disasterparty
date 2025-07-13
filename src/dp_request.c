#define _GNU_SOURCE
#include "dp_private.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

char* build_gemini_count_tokens_json_payload_with_cjson(const dp_request_config_t* request_config) {
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
            } else if (part->type == DP_CONTENT_PART_FILE_REFERENCE) {
                cJSON *file_data_obj = cJSON_CreateObject();
                if (!file_data_obj) {cJSON_Delete(part_obj); cJSON_Delete(root); return NULL;}
                cJSON_AddStringToObject(file_data_obj, "mime_type", part->file_reference.mime_type);
                cJSON_AddStringToObject(file_data_obj, "fileUri", part->file_reference.file_id);
                cJSON_AddItemToObject(part_obj, "fileData", file_data_obj);
            }
            cJSON_AddItemToArray(parts_array, part_obj);
        }
        cJSON_AddItemToArray(contents_array, content_obj);
    }

    char* json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_string; 
}

char* build_anthropic_count_tokens_json_payload_with_cjson(const dp_request_config_t* request_config) {
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
                } else if (part->type == DP_CONTENT_PART_FILE_REFERENCE) {
                    cJSON_AddStringToObject(part_obj, "type", "tool_result");
                    cJSON_AddStringToObject(part_obj, "tool_use_id", part->file_reference.file_id);
                    cJSON *content_array = cJSON_CreateArray();
                    if (!content_array) { cJSON_Delete(part_obj); cJSON_Delete(root); return NULL; }
                    cJSON *text_obj = cJSON_CreateObject();
                    if (!text_obj) { cJSON_Delete(content_array); cJSON_Delete(part_obj); cJSON_Delete(root); return NULL; }
                    cJSON_AddStringToObject(text_obj, "type", "text");
                    cJSON_AddStringToObject(text_obj, "text", "File attached.");
                    cJSON_AddItemToArray(content_array, text_obj);
                    cJSON_AddItemToObject(part_obj, "content", content_array);
                } else if (part->type == DP_CONTENT_PART_FILE_REFERENCE) {
                    cJSON_AddStringToObject(part_obj, "type", "tool_result");
                    cJSON_AddStringToObject(part_obj, "tool_use_id", part->file_reference.file_id);
                    cJSON *content_array = cJSON_CreateArray();
                    if (!content_array) { cJSON_Delete(part_obj); cJSON_Delete(root); return NULL; }
                    cJSON *text_obj = cJSON_CreateObject();
                    if (!text_obj) { cJSON_Delete(content_array); cJSON_Delete(part_obj); cJSON_Delete(root); return NULL; }
                    cJSON_AddStringToObject(text_obj, "type", "text");
                    cJSON_AddStringToObject(text_obj, "text", "File attached.");
                    cJSON_AddItemToArray(content_array, text_obj);
                    cJSON_AddItemToObject(part_obj, "content", content_array);
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

char* build_openai_json_payload_with_cjson(const dp_request_config_t* request_config) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON_AddStringToObject(root, "model", request_config->model);
    if (request_config->temperature >= 0.0) cJSON_AddNumberToObject(root, "temperature", request_config->temperature);
    if (request_config->max_tokens > 0) cJSON_AddNumberToObject(root, "max_tokens", request_config->max_tokens);
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
                cJSON_AddItemToArray(content_array, part_obj);
            }
        }
        cJSON_AddItemToArray(messages_array, msg_obj);
    }

    char* json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_string; 
}

char* build_gemini_json_payload_with_cjson(const dp_request_config_t* request_config) {
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
            } else if (part->type == DP_CONTENT_PART_FILE_REFERENCE) {
                cJSON *file_data_obj = cJSON_CreateObject();
                if (!file_data_obj) {cJSON_Delete(part_obj); cJSON_Delete(root); return NULL;}
                cJSON_AddStringToObject(file_data_obj, "mime_type", part->file_reference.mime_type);
                cJSON_AddStringToObject(file_data_obj, "fileUri", part->file_reference.file_id);
                cJSON_AddItemToObject(part_obj, "fileData", file_data_obj);
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

char* build_anthropic_json_payload_with_cjson(const dp_request_config_t* request_config) {
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
                if(!source_obj) {cJSON_Delete(part_obj); cJSON_Delete(content_array_for_anthropic); cJSON_Delete(root); return NULL;}
                cJSON_AddStringToObject(source_obj, "type", "base64");
                cJSON_AddStringToObject(source_obj, "media_type", part->image_base64.mime_type);
                cJSON_AddStringToObject(source_obj, "data", part->image_base64.data);
                cJSON_AddItemToObject(part_obj, "source", source_obj);
            } else if (part->type == DP_CONTENT_PART_IMAGE_URL) {
                char temp_text[512];
                snprintf(temp_text, sizeof(temp_text), "Image referenced by URL: %s (Anthropic prefers direct image data)", part->image_url);
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

char* extract_text_from_full_response_with_cjson(const char* json_response_str, dp_provider_type_t provider, char** finish_reason_out) {
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
                        extracted_text = dp_internal_strdup(content->valuestring);
                    }
                }
                if (finish_reason_out) {
                    cJSON *reason_item = cJSON_GetObjectItemCaseSensitive(first_choice, "finish_reason");
                    if (cJSON_IsString(reason_item) && reason_item->valuestring) {
                        *finish_reason_out = dp_internal_strdup(reason_item->valuestring);
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
                                extracted_text = dp_internal_strdup(text_item->valuestring);
                                break; 
                            }
                        }
                    }
                }
                if (finish_reason_out) { 
                     cJSON *reason_item = cJSON_GetObjectItemCaseSensitive(first_candidate, "finishReason"); 
                     if (cJSON_IsString(reason_item) && reason_item->valuestring) {
                         *finish_reason_out = dp_internal_strdup(reason_item->valuestring);
                     }
                }
            }
        }
        if (finish_reason_out && !*finish_reason_out) { 
            cJSON *prompt_feedback = cJSON_GetObjectItemCaseSensitive(root, "promptFeedback");
            if (prompt_feedback) {
                cJSON *reason_item = cJSON_GetObjectItemCaseSensitive(prompt_feedback, "finishReason");
                if (cJSON_IsString(reason_item) && reason_item->valuestring) {
                    *finish_reason_out = dp_internal_strdup(reason_item->valuestring);
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
                    extracted_text = dp_internal_strdup(text_item->valuestring);
                }
            }
        }
        if(finish_reason_out) {
            cJSON* reason_item = cJSON_GetObjectItemCaseSensitive(root, "stop_reason");
            if(cJSON_IsString(reason_item) && reason_item->valuestring) {
                *finish_reason_out = dp_internal_strdup(reason_item->valuestring);
            }
        }
    }
    
    cJSON_Delete(root);
    return extracted_text;
}

// --- Request Execution Functions ---

int dp_perform_completion(dp_context_t* context, const dp_request_config_t* request_config, dp_response_t* response) {
    if (!context || !request_config || !response) {
        if (response) response->error_message = dp_internal_strdup("Invalid arguments to dp_perform_completion.");
        return -1;
    }
    if (request_config->stream) {
        if (response) response->error_message = dp_internal_strdup("dp_perform_completion called with stream=true. Use streaming functions instead.");
        return -1;
    }
    memset(response, 0, sizeof(dp_response_t));

    CURL* curl = curl_easy_init();
    if (!curl) {
        response->error_message = dp_internal_strdup("curl_easy_init() failed for Disaster Party completion.");
        return -1;
    }

    char* json_payload_str = NULL;
    if (context->provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
        json_payload_str = build_openai_json_payload_with_cjson(request_config);
    } else if (context->provider == DP_PROVIDER_GOOGLE_GEMINI) {
        json_payload_str = build_gemini_json_payload_with_cjson(request_config);
    } else if (context->provider == DP_PROVIDER_ANTHROPIC) {
        json_payload_str = build_anthropic_json_payload_with_cjson(request_config);
    }
    
    if (!json_payload_str) {
        response->error_message = dp_internal_strdup("Failed to build JSON payload for Disaster Party.");
        curl_easy_cleanup(curl);
        return -1;
    }

    char url[1024];
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    if (context->provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
        snprintf(url, sizeof(url), "%s/chat/completions", context->api_base_url);
        char auth_header[512]; 
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", context->api_key);
        headers = curl_slist_append(headers, auth_header);
    } else if (context->provider == DP_PROVIDER_GOOGLE_GEMINI) { 
        snprintf(url, sizeof(url), "%s/models/%s:generateContent?key=%s",
                 context->api_base_url, request_config->model, context->api_key);
    } else if (context->provider == DP_PROVIDER_ANTHROPIC) {
        snprintf(url, sizeof(url), "%s/messages", context->api_base_url);
        char api_key_header[512];
        snprintf(api_key_header, sizeof(api_key_header), "x-api-key: %s", context->api_key);
        headers = curl_slist_append(headers, api_key_header);
        headers = curl_slist_append(headers, "anthropic-version: 2023-06-01"); 
    }

    memory_struct_t chunk_mem = { .memory = malloc(1), .size = 0 };
    if (!chunk_mem.memory) { 
        response->error_message = dp_internal_strdup("Memory allocation for response chunk failed.");
        free(json_payload_str); curl_slist_free_all(headers); curl_easy_cleanup(curl); return -1; 
    }
    chunk_mem.memory[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload_str);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk_mem);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, context->user_agent);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_status_code);

    if (res != CURLE_OK) {
        asprintf(&response->error_message, "curl_easy_perform() failed: %s (HTTP status: %ld)",
                 curl_easy_strerror(res), response->http_status_code);
    } else {
        if (response->http_status_code >= 200 && response->http_status_code < 300) {
            char* extracted_text = extract_text_from_full_response_with_cjson(chunk_mem.memory, context->provider, &response->finish_reason);
            if (extracted_text) {
                response->parts = calloc(1, sizeof(dp_response_part_t));
                if (response->parts) {
                    response->num_parts = 1;
                    response->parts[0].type = DP_CONTENT_PART_TEXT;
                    response->parts[0].text = extracted_text; 
                } else {
                    free(extracted_text); 
                    response->error_message = dp_internal_strdup("Failed to allocate memory for response part structure.");
                }
            } else { 
                cJSON* error_root = cJSON_Parse(chunk_mem.memory);
                if (error_root) {
                    cJSON* error_obj = cJSON_GetObjectItemCaseSensitive(error_root, "error");
                    if (error_obj) { 
                        cJSON* msg_item = cJSON_GetObjectItemCaseSensitive(error_obj, "message");
                        if (cJSON_IsString(msg_item) && msg_item->valuestring) {
                             asprintf(&response->error_message, "API error (HTTP %ld): %s", response->http_status_code, msg_item->valuestring);
                        }
                    } else { 
                         cJSON* type_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_root, "type");
                         cJSON* msg_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_root, "message"); 
                         if(cJSON_IsString(type_item_anthropic) && strcmp(type_item_anthropic->valuestring, "error") == 0 &&
                            cJSON_IsString(msg_item_anthropic) && msg_item_anthropic->valuestring) {
                            asprintf(&response->error_message, "API error (HTTP %ld): %s", response->http_status_code, msg_item_anthropic->valuestring);
                         }
                    }
                    cJSON_Delete(error_root);
                }
                if (!response->error_message && chunk_mem.memory) { 
                    asprintf(&response->error_message, "Failed to parse successful response or extract text (HTTP %ld). Body: %.200s...", response->http_status_code, chunk_mem.memory);
                } else if (!response->error_message) {
                    asprintf(&response->error_message, "Failed to parse successful response or extract text (HTTP %ld). Empty response body.", response->http_status_code);
                }
            }
        } else { 
             cJSON* error_root = cJSON_Parse(chunk_mem.memory);
             char* api_err_detail = NULL;
             if(error_root){
                cJSON* error_obj = cJSON_GetObjectItemCaseSensitive(error_root, "error");
                if(error_obj){
                    cJSON* msg_item = cJSON_GetObjectItemCaseSensitive(error_obj, "message");
                    if(cJSON_IsString(msg_item) && msg_item->valuestring){
                        api_err_detail = msg_item->valuestring; 
                    }
                } else {
                     cJSON* type_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_root, "type");
                     cJSON* msg_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_root, "message");
                     if(cJSON_IsString(type_item_anthropic) && strcmp(type_item_anthropic->valuestring, "error") == 0 &&
                        cJSON_IsString(msg_item_anthropic) && msg_item_anthropic->valuestring) {
                        api_err_detail = msg_item_anthropic->valuestring;
                     }
                }
             }
            if(api_err_detail){
                asprintf(&response->error_message, "HTTP error %ld: %s", response->http_status_code, api_err_detail);
            } else if (chunk_mem.memory) {
                asprintf(&response->error_message, "HTTP error %ld. Body: %.500s", response->http_status_code, chunk_mem.memory);
            } else {
                asprintf(&response->error_message, "HTTP error %ld. (no response body)", response->http_status_code);
            }
            if(error_root) cJSON_Delete(error_root);
        }
    }

    free(json_payload_str);
    free(chunk_mem.memory);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response->error_message ? -1 : 0;
}

int dp_count_tokens(dp_context_t* context,
                    const dp_request_config_t* request_config,
                    size_t* token_count_out) {
    if (!context || !request_config || !token_count_out) {
        return -1;
    }
    *token_count_out = 0;

    if (context->provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
        return DP_ERROR_TOKEN_COUNTING_NOT_SUPPORTED;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "curl_easy_init() failed for dp_count_tokens.\n");
        return -1;
    }

    char* json_payload_str = NULL;
    char url[1024];
    struct curl_slist* headers = NULL;
    int return_code = -1;
    long http_status_code = 0;
    memory_struct_t chunk_mem = { .memory = NULL, .size = 0 };


    headers = curl_slist_append(headers, "Content-Type: application/json");

    switch (context->provider) {
        case DP_PROVIDER_GOOGLE_GEMINI:
            json_payload_str = build_gemini_count_tokens_json_payload_with_cjson(request_config);
            if (!json_payload_str) {
                fprintf(stderr, "Failed to build JSON payload for dp_count_tokens (Gemini).\n");
                goto cleanup;
            }
            snprintf(url, sizeof(url), "%s/models/%s:countTokens?key=%s",
                     context->api_base_url, request_config->model, context->api_key);
            break;
        case DP_PROVIDER_ANTHROPIC:
            json_payload_str = build_anthropic_count_tokens_json_payload_with_cjson(request_config);
            if (!json_payload_str) {
                fprintf(stderr, "Failed to build JSON payload for dp_count_tokens (Anthropic).\n");
                goto cleanup;
            }
            snprintf(url, sizeof(url), "%s/messages/count_tokens", context->api_base_url);
            char api_key_header[512];
            snprintf(api_key_header, sizeof(api_key_header), "x-api-key: %s", context->api_key);
            headers = curl_slist_append(headers, api_key_header);
            headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
            break;
        default:
            fprintf(stderr, "Unsupported provider for dp_count_tokens.\n");
            goto cleanup;
    }

    chunk_mem.memory = malloc(1);
    if (!chunk_mem.memory) {
        fprintf(stderr, "Memory allocation for response chunk failed in dp_count_tokens.\n");
        goto cleanup;
    }
    chunk_mem.memory[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload_str);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk_mem);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, context->user_agent);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status_code);

    if (res == CURLE_OK && http_status_code >= 200 && http_status_code < 300) {
        cJSON *root = cJSON_Parse(chunk_mem.memory);
        if (root) {
            cJSON *total_tokens_item = NULL;
            if (context->provider == DP_PROVIDER_GOOGLE_GEMINI) {
                total_tokens_item = cJSON_GetObjectItemCaseSensitive(root, "totalTokens");
            } else if (context->provider == DP_PROVIDER_ANTHROPIC) {
                total_tokens_item = cJSON_GetObjectItemCaseSensitive(root, "input_tokens");
            }

            if (cJSON_IsNumber(total_tokens_item)) {
                *token_count_out = (size_t)total_tokens_item->valuedouble;
                return_code = 0;
            }
            cJSON_Delete(root);
        } else {
            fprintf(stderr, "Failed to parse JSON response for token count.\n");
        }
    } else {
        fprintf(stderr, "API call for token count failed: %s (HTTP status: %ld)\n", curl_easy_strerror(res), http_status_code);
        if (chunk_mem.memory) {
            fprintf(stderr, "Response body: %s\n", chunk_mem.memory);
        }
    }

cleanup:
    free(json_payload_str);
    if (chunk_mem.memory) free(chunk_mem.memory);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return return_code;
}
