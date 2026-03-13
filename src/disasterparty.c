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
    if (request_config->reasoning_effort) cJSON_AddStringToObject(root, "reasoning_effort", request_config->reasoning_effort);
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

    if (request_config->tools && request_config->num_tools > 0) {
        cJSON* tools_array = cJSON_AddArrayToObject(root, "tools");
        for (size_t i = 0; i < request_config->num_tools; ++i) {
            cJSON* tool_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(tool_obj, "type", "function");
            
            cJSON* func_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(func_obj, "name", request_config->tools[i].function.name);
            cJSON_AddStringToObject(func_obj, "description", request_config->tools[i].function.description);
            
            if (request_config->tools[i].function.parameters_json_schema) {
                 cJSON* params = cJSON_Parse(request_config->tools[i].function.parameters_json_schema);
                 if (params) {
                     cJSON_AddItemToObject(func_obj, "parameters", params);
                 }
            }
            
            cJSON_AddItemToObject(tool_obj, "function", func_obj);
            cJSON_AddItemToArray(tools_array, tool_obj);
        }
        
        if (request_config->tool_choice.type == DP_TOOL_CHOICE_NONE) {
            cJSON_AddStringToObject(root, "tool_choice", "none");
        } else if (request_config->tool_choice.type == DP_TOOL_CHOICE_ANY) {
            cJSON_AddStringToObject(root, "tool_choice", "required");
        } else if (request_config->tool_choice.type == DP_TOOL_CHOICE_TOOL) {
            cJSON* choice_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(choice_obj, "type", "function");
            cJSON* func_choice = cJSON_CreateObject();
            cJSON_AddStringToObject(func_choice, "name", request_config->tool_choice.tool_name);
            cJSON_AddItemToObject(choice_obj, "function", func_choice);
            cJSON_AddItemToObject(root, "tool_choice", choice_obj);
        }
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

        if (msg->role == DP_ROLE_TOOL) {
            for (size_t j = 0; j < msg->num_parts; ++j) {
                if (msg->parts[j].type == DP_CONTENT_PART_TOOL_RESULT) {
                    cJSON_AddStringToObject(msg_obj, "content", msg->parts[j].tool_result.content);
                    cJSON_AddStringToObject(msg_obj, "tool_call_id", msg->parts[j].tool_result.tool_call_id);
                    break;
                }
            }
        } else if (msg->num_parts == 1 && msg->parts[0].type == DP_CONTENT_PART_TEXT) {
            cJSON_AddStringToObject(msg_obj, "content", msg->parts[0].text);
        } else {
            cJSON *content_array = NULL;
            cJSON *tool_calls_array = NULL;

            for (size_t j = 0; j < msg->num_parts; ++j) {
                const dp_content_part_t* part = &msg->parts[j];

                if (part->type == DP_CONTENT_PART_TOOL_CALL) {
                    if (!tool_calls_array) tool_calls_array = cJSON_AddArrayToObject(msg_obj, "tool_calls");
                    cJSON* tc_obj = cJSON_CreateObject();
                    cJSON_AddStringToObject(tc_obj, "id", part->tool_call.id);
                    cJSON_AddStringToObject(tc_obj, "type", "function");
                    cJSON* func_obj = cJSON_CreateObject();
                    cJSON_AddStringToObject(func_obj, "name", part->tool_call.function_name);
                    cJSON_AddStringToObject(func_obj, "arguments", part->tool_call.arguments_json);
                    cJSON_AddItemToObject(tc_obj, "function", func_obj);
                    cJSON_AddItemToArray(tool_calls_array, tc_obj);
                    continue;
                }

                if (!content_array) {
                    content_array = cJSON_AddArrayToObject(msg_obj, "content");
                    if (!content_array) { cJSON_Delete(root); return NULL; }
                }

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

    if (request_config->tools && request_config->num_tools > 0) {
        cJSON* tools_array = cJSON_AddArrayToObject(root, "tools");
        cJSON* func_decls_wrapper = cJSON_CreateObject();
        cJSON* func_decls = cJSON_AddArrayToObject(func_decls_wrapper, "function_declarations");
        
        for (size_t i = 0; i < request_config->num_tools; ++i) {
            cJSON* func_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(func_obj, "name", request_config->tools[i].function.name);
            cJSON_AddStringToObject(func_obj, "description", request_config->tools[i].function.description);
            if (request_config->tools[i].function.parameters_json_schema) {
                 cJSON* params = cJSON_Parse(request_config->tools[i].function.parameters_json_schema);
                 if (params) {
                     cJSON_AddItemToObject(func_obj, "parameters", params);
                 }
            }
            cJSON_AddItemToArray(func_decls, func_obj);
        }
        cJSON_AddItemToArray(tools_array, func_decls_wrapper);

        if (request_config->tool_choice.type != DP_TOOL_CHOICE_AUTO) {
            cJSON* tool_config = cJSON_AddObjectToObject(root, "toolConfig");
            cJSON* func_calling_config = cJSON_AddObjectToObject(tool_config, "functionCallingConfig");
            if (request_config->tool_choice.type == DP_TOOL_CHOICE_ANY) {
                cJSON_AddStringToObject(func_calling_config, "mode", "ANY");
            } else if (request_config->tool_choice.type == DP_TOOL_CHOICE_NONE) {
                 cJSON_AddStringToObject(func_calling_config, "mode", "NONE");
            } else if (request_config->tool_choice.type == DP_TOOL_CHOICE_TOOL) {
                 cJSON_AddStringToObject(func_calling_config, "mode", "ANY");
                 cJSON* allowed = cJSON_AddArrayToObject(func_calling_config, "allowedFunctionNames");
                 cJSON_AddItemToArray(allowed, cJSON_CreateString(request_config->tool_choice.tool_name));
            }
        }
    }

    cJSON *contents_array = cJSON_AddArrayToObject(root, "contents");
    if (!contents_array) { cJSON_Delete(root); return NULL; }

    for (size_t i = 0; i < request_config->num_messages; ++i) {
        const dp_message_t* msg = &request_config->messages[i];
        if (msg->role == DP_ROLE_SYSTEM) continue;

        cJSON *content_obj = cJSON_CreateObject();
        if (!content_obj) { cJSON_Delete(root); return NULL; }

        const char* role_str;
        if (msg->role == DP_ROLE_ASSISTANT) role_str = "model";
        else if (msg->role == DP_ROLE_TOOL) role_str = "function";
        else role_str = "user";

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
            } else if (part->type == DP_CONTENT_PART_TOOL_CALL) {
                cJSON* func_call = cJSON_CreateObject();
                cJSON_AddStringToObject(func_call, "name", part->tool_call.function_name);
                cJSON* args = cJSON_Parse(part->tool_call.arguments_json);
                if (args) cJSON_AddItemToObject(func_call, "args", args);
                else cJSON_AddItemToObject(func_call, "args", cJSON_CreateObject());
                cJSON_AddItemToObject(part_obj, "functionCall", func_call);
            } else if (part->type == DP_CONTENT_PART_TOOL_RESULT) {
                cJSON* func_resp = cJSON_CreateObject();
                // For Gemini, we use tool_call_id as the function name
                cJSON_AddStringToObject(func_resp, "name", part->tool_result.tool_call_id);
                
                cJSON* response_content = cJSON_Parse(part->tool_result.content);
                cJSON* resp_wrapper = cJSON_CreateObject();
                
                if (response_content) {
                     cJSON_AddItemToObject(resp_wrapper, "content", response_content);
                } else {
                     // If not JSON, treat as simple string
                     cJSON_AddStringToObject(resp_wrapper, "content", part->tool_result.content);
                }
                cJSON_AddItemToObject(func_resp, "response", resp_wrapper);
                cJSON_AddItemToObject(part_obj, "functionResponse", func_resp);
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

    if (request_config->thinking.enabled) {
        cJSON* thinking_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(thinking_obj, "type", "enabled");
        cJSON_AddNumberToObject(thinking_obj, "budget_tokens", request_config->thinking.budget_tokens > 0 ? request_config->thinking.budget_tokens : 1024);
        cJSON_AddItemToObject(root, "thinking", thinking_obj);
    }

    if (request_config->system_prompt && strlen(request_config->system_prompt) > 0) {
        cJSON_AddStringToObject(root, "system", request_config->system_prompt);
    }

    if (request_config->tools && request_config->num_tools > 0) {
        cJSON* tools_array = cJSON_AddArrayToObject(root, "tools");
        for (size_t i = 0; i < request_config->num_tools; ++i) {
            cJSON* tool_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(tool_obj, "name", request_config->tools[i].function.name);
            cJSON_AddStringToObject(tool_obj, "description", request_config->tools[i].function.description);
            if (request_config->tools[i].function.parameters_json_schema) {
                 cJSON* params = cJSON_Parse(request_config->tools[i].function.parameters_json_schema);
                 if (params) {
                     cJSON_AddItemToObject(tool_obj, "input_schema", params);
                 }
            }
            cJSON_AddItemToArray(tools_array, tool_obj);
        }

        if (request_config->tool_choice.type == DP_TOOL_CHOICE_ANY) {
            cJSON* tc = cJSON_CreateObject();
            cJSON_AddStringToObject(tc, "type", "any");
            cJSON_AddItemToObject(root, "tool_choice", tc);
        } else if (request_config->tool_choice.type == DP_TOOL_CHOICE_TOOL) {
            cJSON* tc = cJSON_CreateObject();
            cJSON_AddStringToObject(tc, "type", "tool");
            cJSON_AddStringToObject(tc, "name", request_config->tool_choice.tool_name);
            cJSON_AddItemToObject(root, "tool_choice", tc);
        }
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
            } else if (part->type == DP_CONTENT_PART_TOOL_CALL) {
                cJSON_AddStringToObject(part_obj, "type", "tool_use");
                cJSON_AddStringToObject(part_obj, "id", part->tool_call.id);
                cJSON_AddStringToObject(part_obj, "name", part->tool_call.function_name);
                cJSON* input_json = cJSON_Parse(part->tool_call.arguments_json);
                if (input_json) cJSON_AddItemToObject(part_obj, "input", input_json);
                else cJSON_AddItemToObject(part_obj, "input", cJSON_CreateObject());
            } else if (part->type == DP_CONTENT_PART_TOOL_RESULT) {
                cJSON_AddStringToObject(part_obj, "type", "tool_result");
                cJSON_AddStringToObject(part_obj, "tool_use_id", part->tool_result.tool_call_id);
                cJSON_AddStringToObject(part_obj, "content", part->tool_result.content);
                if (part->tool_result.is_error) {
                    cJSON_AddTrueToObject(part_obj, "is_error");
                }
            } else if (part->type == DP_CONTENT_PART_THINKING) {
                cJSON_AddStringToObject(part_obj, "type", "thinking");
                cJSON_AddStringToObject(part_obj, "thinking", part->thinking.thinking);
                cJSON_AddStringToObject(part_obj, "signature", part->thinking.signature);
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

char* dpinternal_build_openai_image_generation_payload_with_cjson(const dp_image_generation_config_t* config) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON_AddStringToObject(root, "prompt", config->prompt);
    cJSON_AddStringToObject(root, "model", config->model ? config->model : "dall-e-3");
    
    if (config->n > 0) cJSON_AddNumberToObject(root, "n", config->n);
    else cJSON_AddNumberToObject(root, "n", 1);
    
    if (config->size) cJSON_AddStringToObject(root, "size", config->size);
    else cJSON_AddStringToObject(root, "size", "1024x1024");

    if (config->quality) cJSON_AddStringToObject(root, "quality", config->quality);
    if (config->style) cJSON_AddStringToObject(root, "style", config->style);
    
    if (config->response_format) cJSON_AddStringToObject(root, "response_format", config->response_format);

    char* json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_string;
}

char* dpinternal_build_google_image_generation_payload_with_cjson(const dp_image_generation_config_t* config, const dp_context_t* context) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON *instances = cJSON_AddArrayToObject(root, "instances");
    cJSON *inst = cJSON_CreateObject();
    cJSON_AddStringToObject(inst, "prompt", config->prompt);
    cJSON_AddItemToArray(instances, inst);

    cJSON *parameters = cJSON_AddObjectToObject(root, "parameters");
    cJSON_AddNumberToObject(parameters, "sampleCount", config->n > 0 ? config->n : 1);

    if (config->size) cJSON_AddStringToObject(parameters, "aspectRatio", config->size);
    
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



bool dpinternal_parse_response_content(const dp_context_t* context, const char* json_response_str, dp_response_part_t** parts_out, size_t* num_parts_out, char** finish_reason_out) {
    if (finish_reason_out) *finish_reason_out = NULL;
    if (parts_out) *parts_out = NULL;
    if (num_parts_out) *num_parts_out = 0;
    if (!json_response_str || !context) return false;

    dp_provider_type_t provider = context->provider;
    cJSON *root = cJSON_Parse(json_response_str);
    if (!root) return false;

    dp_response_part_t* parts = NULL;
    size_t num_parts = 0;

    if (provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
        cJSON* choices = cJSON_GetObjectItemCaseSensitive(root, "choices");
        if (cJSON_IsArray(choices) && cJSON_GetArraySize(choices) > 0) {
            cJSON* first_choice = cJSON_GetArrayItem(choices, 0);
            cJSON* message = cJSON_GetObjectItemCaseSensitive(first_choice, "message");
            if (message) {
                cJSON* content = cJSON_GetObjectItemCaseSensitive(message, "content");
                if (cJSON_IsString(content) && content->valuestring) {
                    parts = realloc(parts, (num_parts + 1) * sizeof(dp_response_part_t));
                    memset(&parts[num_parts], 0, sizeof(dp_response_part_t));
                    parts[num_parts].type = DP_CONTENT_PART_TEXT;
                    parts[num_parts].text = dpinternal_strdup(content->valuestring);
                    num_parts++;
                }
                cJSON* tool_calls = cJSON_GetObjectItemCaseSensitive(message, "tool_calls");
                if (cJSON_IsArray(tool_calls)) {
                    cJSON* tc = NULL;
                    cJSON_ArrayForEach(tc, tool_calls) {
                        cJSON* id = cJSON_GetObjectItemCaseSensitive(tc, "id");
                        cJSON* func = cJSON_GetObjectItemCaseSensitive(tc, "function");
                        if (cJSON_IsString(id) && func) {
                            cJSON* name = cJSON_GetObjectItemCaseSensitive(func, "name");
                            cJSON* args = cJSON_GetObjectItemCaseSensitive(func, "arguments");
                            if (cJSON_IsString(name) && cJSON_IsString(args)) {
                                parts = realloc(parts, (num_parts + 1) * sizeof(dp_response_part_t));
                                memset(&parts[num_parts], 0, sizeof(dp_response_part_t));
                                parts[num_parts].type = DP_CONTENT_PART_TOOL_CALL;
                                parts[num_parts].tool_call.id = dpinternal_strdup(id->valuestring);
                                parts[num_parts].tool_call.function_name = dpinternal_strdup(name->valuestring);
                                parts[num_parts].tool_call.arguments_json = dpinternal_strdup(args->valuestring);
                                num_parts++;
                            }
                        }
                    }
                }
            }
            if (finish_reason_out) {
                cJSON* reason = cJSON_GetObjectItemCaseSensitive(first_choice, "finish_reason");
                if (cJSON_IsString(reason) && reason->valuestring) {
                    *finish_reason_out = dpinternal_strdup(reason->valuestring);
                }
            }
        }
    } else if (provider == DP_PROVIDER_GOOGLE_GEMINI) {
        cJSON* candidates = cJSON_GetObjectItemCaseSensitive(root, "candidates");
        if (cJSON_IsArray(candidates) && cJSON_GetArraySize(candidates) > 0) {
            cJSON* first_candidate = cJSON_GetArrayItem(candidates, 0);
            cJSON* content = cJSON_GetObjectItemCaseSensitive(first_candidate, "content");
            if (content) {
                cJSON* parts_array = cJSON_GetObjectItemCaseSensitive(content, "parts");
                if (cJSON_IsArray(parts_array)) {
                    cJSON* part = NULL;
                    cJSON_ArrayForEach(part, parts_array) {
                        cJSON* text = cJSON_GetObjectItemCaseSensitive(part, "text");
                        cJSON* func_call = cJSON_GetObjectItemCaseSensitive(part, "functionCall");
                        cJSON *thought_item = cJSON_GetObjectItemCaseSensitive(part, "thought");

                        if (cJSON_IsTrue(thought_item)) {
                            if (context->features & (1ULL << (DP_FEATURE_THINKING - 1))) {
                                if (cJSON_IsString(text) && text->valuestring) {
                                    parts = realloc(parts, (num_parts + 1) * sizeof(dp_response_part_t));
                                    memset(&parts[num_parts], 0, sizeof(dp_response_part_t));
                                    parts[num_parts].type = DP_CONTENT_PART_THINKING;
                                    parts[num_parts].thinking.thinking = dpinternal_strdup(text->valuestring);
                                    num_parts++;
                                }
                            }
                            continue;
                        }

                        if (cJSON_IsString(text) && text->valuestring) {
                            parts = realloc(parts, (num_parts + 1) * sizeof(dp_response_part_t));
                            memset(&parts[num_parts], 0, sizeof(dp_response_part_t));
                            parts[num_parts].type = DP_CONTENT_PART_TEXT;
                            parts[num_parts].text = dpinternal_strdup(text->valuestring);
                            num_parts++;
                        } else if (func_call) {
                            cJSON* name = cJSON_GetObjectItemCaseSensitive(func_call, "name");
                            cJSON* args = cJSON_GetObjectItemCaseSensitive(func_call, "args");
                            if (cJSON_IsString(name)) {
                                parts = realloc(parts, (num_parts + 1) * sizeof(dp_response_part_t));
                                memset(&parts[num_parts], 0, sizeof(dp_response_part_t));
                                parts[num_parts].type = DP_CONTENT_PART_TOOL_CALL;
                                parts[num_parts].tool_call.id = dpinternal_strdup(name->valuestring);
                                parts[num_parts].tool_call.function_name = dpinternal_strdup(name->valuestring);
                                parts[num_parts].tool_call.arguments_json = cJSON_PrintUnformatted(args ? args : cJSON_CreateObject());
                                num_parts++;
                            }
                        }
                    }
                }
            }
            if (finish_reason_out) {
                cJSON* reason = cJSON_GetObjectItemCaseSensitive(first_candidate, "finishReason");
                if (cJSON_IsString(reason) && reason->valuestring) {
                    *finish_reason_out = dpinternal_strdup(reason->valuestring);
                }
            }
        }
        if (finish_reason_out && !*finish_reason_out) { 
            cJSON *prompt_feedback = cJSON_GetObjectItemCaseSensitive(root, "promptFeedback");
            if (prompt_feedback) {
                cJSON *reason_item = cJSON_GetObjectItemCaseSensitive(prompt_feedback, "blockReason");
                if (!reason_item) reason_item = cJSON_GetObjectItemCaseSensitive(prompt_feedback, "finishReason");
                if (cJSON_IsString(reason_item) && reason_item->valuestring) {
                    *finish_reason_out = dpinternal_strdup(reason_item->valuestring);
                }
            }
        }
    } else if (provider == DP_PROVIDER_ANTHROPIC) {
        cJSON* content = cJSON_GetObjectItemCaseSensitive(root, "content");
        if (cJSON_IsArray(content)) {
            cJSON* block = NULL;
            cJSON_ArrayForEach(block, content) {
                cJSON* type = cJSON_GetObjectItemCaseSensitive(block, "type");
                if (cJSON_IsString(type)) {
                    if (strcmp(type->valuestring, "text") == 0) {
                        cJSON* text = cJSON_GetObjectItemCaseSensitive(block, "text");
                        if (cJSON_IsString(text) && text->valuestring) {
                            parts = realloc(parts, (num_parts + 1) * sizeof(dp_response_part_t));
                            memset(&parts[num_parts], 0, sizeof(dp_response_part_t));
                            parts[num_parts].type = DP_CONTENT_PART_TEXT;
                            parts[num_parts].text = dpinternal_strdup(text->valuestring);
                            num_parts++;
                        }
                    } else if (strcmp(type->valuestring, "tool_use") == 0) {
                        cJSON* id = cJSON_GetObjectItemCaseSensitive(block, "id");
                        cJSON* name = cJSON_GetObjectItemCaseSensitive(block, "name");
                        cJSON* input = cJSON_GetObjectItemCaseSensitive(block, "input");
                        if (cJSON_IsString(id) && cJSON_IsString(name)) {
                            parts = realloc(parts, (num_parts + 1) * sizeof(dp_response_part_t));
                            memset(&parts[num_parts], 0, sizeof(dp_response_part_t));
                            parts[num_parts].type = DP_CONTENT_PART_TOOL_CALL;
                            parts[num_parts].tool_call.id = dpinternal_strdup(id->valuestring);
                            parts[num_parts].tool_call.function_name = dpinternal_strdup(name->valuestring);
                            parts[num_parts].tool_call.arguments_json = cJSON_PrintUnformatted(input ? input : cJSON_CreateObject());
                            num_parts++;
                        }
                    } else if (strcmp(type->valuestring, "thinking") == 0) {
                        cJSON* thinking = cJSON_GetObjectItemCaseSensitive(block, "thinking");
                        cJSON* signature = cJSON_GetObjectItemCaseSensitive(block, "signature");
                        if (cJSON_IsString(thinking) && thinking->valuestring && cJSON_IsString(signature) && signature->valuestring) {
                            parts = realloc(parts, (num_parts + 1) * sizeof(dp_response_part_t));
                            memset(&parts[num_parts], 0, sizeof(dp_response_part_t));
                            parts[num_parts].type = DP_CONTENT_PART_THINKING;
                            parts[num_parts].thinking.thinking = dpinternal_strdup(thinking->valuestring);
                            parts[num_parts].thinking.signature = dpinternal_strdup(signature->valuestring);
                            num_parts++;
                        }
                    }
                }
            }
        }
        if (finish_reason_out) {
            cJSON* reason = cJSON_GetObjectItemCaseSensitive(root, "stop_reason");
            if (cJSON_IsString(reason) && reason->valuestring) {
                *finish_reason_out = dpinternal_strdup(reason->valuestring);
            }
        }
    }

    cJSON_Delete(root);
    *parts_out = parts;
    *num_parts_out = num_parts;
    return true;
}

CURLcode dpinternal_perform_openai_streaming_request_with_fallback(CURL* curl, dp_context_t* context, 
                                                                        const dp_request_config_t* request_config,
                                                                        stream_processor_t* processor,
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
        processor->accumulated_error_during_stream && 
        dpinternal_is_token_parameter_error(processor->accumulated_error_during_stream, *http_status_code)) {
        
        // Switch to legacy parameter and retry
        context->token_param_preference = DP_TOKEN_PARAM_MAX_TOKENS;
        free(json_payload_str);
        
        // Reset response buffer for retry
        if (processor->accumulated_error_during_stream) {
            free(processor->accumulated_error_during_stream);
            processor->accumulated_error_during_stream = NULL;
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










void dp_free_image_generation_response(dp_image_generation_response_t* response) {
    if (!response) return;
    if (response->images) {
        for (size_t i = 0; i < response->num_images; ++i) {
            free(response->images[i].url);
            free(response->images[i].base64_json);
            free(response->images[i].revised_prompt);
        }
        free(response->images);
    }
    free(response->error_message);
    memset(response, 0, sizeof(dp_image_generation_response_t));
}

void dp_free_response_content(dp_response_t* response) {
    if (!response) return;
    if (response->parts) {
        for (size_t i = 0; i < response->num_parts; ++i) {
            free(response->parts[i].text); 
            if (response->parts[i].type == DP_CONTENT_PART_TOOL_CALL) {
                free(response->parts[i].tool_call.id);
                free(response->parts[i].tool_call.function_name);
                free(response->parts[i].tool_call.arguments_json);
            } else if (response->parts[i].type == DP_CONTENT_PART_THINKING) {
                free(response->parts[i].thinking.thinking);
                free(response->parts[i].thinking.signature);
            }
        }
        free(response->parts);
    }
    free(response->error_message);
    free(response->finish_reason);
    memset(response, 0, sizeof(dp_response_t)); 
}

CURLcode dpinternal_perform_openai_detailed_streaming_request_with_fallback(CURL* curl, dp_context_t* context, 
                                                                        const dp_request_config_t* request_config,
                                                                        anthropic_stream_processor_t* processor,
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
        processor->accumulated_error_during_stream && 
        dpinternal_is_token_parameter_error(processor->accumulated_error_during_stream, *http_status_code)) {
        
        // Switch to legacy parameter and retry
        context->token_param_preference = DP_TOKEN_PARAM_MAX_TOKENS;
        free(json_payload_str);
        
        // Reset response buffer for retry
        if (processor->accumulated_error_during_stream) {
            free(processor->accumulated_error_during_stream);
            processor->accumulated_error_during_stream = NULL;
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






