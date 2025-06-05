#define _GNU_SOURCE
#include "disasterparty.h"
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// Default base URLs
const char* DEFAULT_OPENAI_API_BASE_URL = "https://api.openai.com/v1";
const char* DEFAULT_GEMINI_API_BASE_URL = "https://generativelanguage.googleapis.com/v1beta";
const char* DEFAULT_ANTHROPIC_API_BASE_URL = "https://api.anthropic.com/v1"; // Anthropic base URL
const char* DISASTERPARTY_USER_AGENT = "disasterparty/" DP_VERSION;


// Internal context structure
struct dp_context_s {
    dp_provider_type_t provider;
    char* api_key;
    char* api_base_url;
    // For Anthropic, we might need to store the API version header value if it's configurable
    // For now, we'll hardcode "2023-06-01" as it's common.
};

typedef struct {
    char* memory;
    size_t size;
} memory_struct_t;

// Stream processor for generic text-token callback
typedef struct {
    dp_stream_callback_t user_callback; // Generic callback
    void* user_data;
    char* buffer;
    size_t buffer_size;
    size_t buffer_capacity;
    dp_provider_type_t provider;
    char* finish_reason_capture;
    bool stop_streaming_signal;
    char* accumulated_error_during_stream;
} stream_processor_t;

// Stream processor for detailed Anthropic event callback
typedef struct {
    dp_anthropic_stream_callback_t anthropic_user_callback; // Anthropic-specific callback
    void* user_data;
    char* buffer;
    size_t buffer_size;
    size_t buffer_capacity;
    // No provider needed here as this processor is Anthropic-specific
    char* finish_reason_capture; // For overall status, though detailed events might have their own
    bool stop_streaming_signal;
    char* accumulated_error_during_stream;
} anthropic_stream_processor_t;


const char* dp_get_version(void) {
    return DP_VERSION;
}

static char* dp_internal_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* new_s = malloc(len);
    if (!new_s) return NULL;
    memcpy(new_s, s, len);
    return new_s;
}

static size_t write_memory_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    memory_struct_t* mem = (memory_struct_t*)userp;
    char* ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        fprintf(stderr, "write_memory_callback: not enough memory (realloc returned NULL)\n");
        return 0;
    }
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

dp_context_t* dp_init_context(dp_provider_type_t provider, const char* api_key, const char* api_base_url) {
    if (!api_key) {
        fprintf(stderr, "API key is required for Disaster Party context.\n");
        return NULL;
    }
    dp_context_t* context = calloc(1, sizeof(dp_context_t));
    if (!context) {
        perror("Failed to allocate Disaster Party context");
        return NULL;
    }
    context->provider = provider;
    context->api_key = dp_internal_strdup(api_key);

    if (api_base_url) {
        context->api_base_url = dp_internal_strdup(api_base_url);
    } else {
        switch (provider) {
            case DP_PROVIDER_OPENAI_COMPATIBLE:
                context->api_base_url = dp_internal_strdup(DEFAULT_OPENAI_API_BASE_URL);
                break;
            case DP_PROVIDER_GOOGLE_GEMINI:
                context->api_base_url = dp_internal_strdup(DEFAULT_GEMINI_API_BASE_URL);
                break;
            case DP_PROVIDER_ANTHROPIC:
                context->api_base_url = dp_internal_strdup(DEFAULT_ANTHROPIC_API_BASE_URL);
                break;
            default:
                // Should not happen if enum is handled correctly
                context->api_base_url = NULL;
                break;
        }
    }

    if (!context->api_key || !context->api_base_url) {
        perror("Failed to allocate API key or base URL in Disaster Party context");
        free(context->api_key);
        free(context->api_base_url);
        free(context);
        return NULL;
    }
    return context;
}

void dp_destroy_context(dp_context_t* context) {
    if (!context) return;
    free(context->api_key);
    free(context->api_base_url);
    free(context);
}

static char* build_openai_json_payload_with_cjson(const dp_request_config_t* request_config) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON_AddStringToObject(root, "model", request_config->model);
    cJSON_AddNumberToObject(root, "temperature", request_config->temperature);
    if (request_config->max_tokens > 0) {
        cJSON_AddNumberToObject(root, "max_tokens", request_config->max_tokens);
    }
    if (request_config->stream) {
        cJSON_AddTrueToObject(root, "stream");
    }

    cJSON *messages_array = cJSON_AddArrayToObject(root, "messages");
    if (!messages_array) {
        cJSON_Delete(root);
        return NULL;
    }

    for (size_t i = 0; i < request_config->num_messages; ++i) {
        const dp_message_t* msg = &request_config->messages[i];
        cJSON *msg_obj = cJSON_CreateObject();
        if (!msg_obj) { cJSON_Delete(root); return NULL; }

        const char* role_str = NULL;
        switch (msg->role) {
            case DP_ROLE_SYSTEM: role_str = "system"; break;
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

static char* build_gemini_json_payload_with_cjson(const dp_request_config_t* request_config) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON *contents_array = cJSON_AddArrayToObject(root, "contents");
    if (!contents_array) { cJSON_Delete(root); return NULL; }

    for (size_t i = 0; i < request_config->num_messages; ++i) {
        const dp_message_t* msg = &request_config->messages[i];
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
            }
            cJSON_AddItemToArray(parts_array, part_obj);
        }
        cJSON_AddItemToArray(contents_array, content_obj);
    }

    cJSON *gen_config = cJSON_AddObjectToObject(root, "generationConfig");
    if (!gen_config) { cJSON_Delete(root); return NULL; }
    cJSON_AddNumberToObject(gen_config, "temperature", request_config->temperature);
    if (request_config->max_tokens > 0) {
        cJSON_AddNumberToObject(gen_config, "maxOutputTokens", request_config->max_tokens);
    }

    char* json_string = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_string;
}

static char* build_anthropic_json_payload_with_cjson(const dp_request_config_t* request_config) {
    cJSON *root = cJSON_CreateObject();
    if (!root) return NULL;

    cJSON_AddStringToObject(root, "model", request_config->model);
    cJSON_AddNumberToObject(root, "max_tokens", request_config->max_tokens > 0 ? request_config->max_tokens : 1024); // Anthropic requires max_tokens
    if (request_config->temperature >= 0.0 && request_config->temperature <= 1.0) { // Anthropic specific range for messages API often 0-1
        cJSON_AddNumberToObject(root, "temperature", request_config->temperature);
    } else if (request_config->temperature > 1.0) { // Default if out of typical range
         cJSON_AddNumberToObject(root, "temperature", 1.0);
    }


    if (request_config->system_prompt && strlen(request_config->system_prompt) > 0) {
        cJSON_AddStringToObject(root, "system", request_config->system_prompt);
    }

    cJSON *messages_array = cJSON_AddArrayToObject(root, "messages");
    if (!messages_array) { cJSON_Delete(root); return NULL; }

    for (size_t i = 0; i < request_config->num_messages; ++i) {
        const dp_message_t* msg = &request_config->messages[i];
        if (msg->role == DP_ROLE_SYSTEM) continue; // System prompts handled separately for Anthropic

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
                // Anthropic generally prefers base64 for images in messages.
                // If an image URL is given, we might convert it to a text description as a fallback.
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


static char* extract_text_from_full_response_with_cjson(const char* json_response_str, dp_provider_type_t provider, char** finish_reason_out) {
    if (finish_reason_out) *finish_reason_out = NULL;
    if (!json_response_str) return NULL;

    cJSON *root = cJSON_Parse(json_response_str);
    if (!root) {
        return NULL;
    }

    char* extracted_text = NULL;
    cJSON *item_array = NULL; // For OpenAI 'choices', Gemini 'candidates', Anthropic 'content'

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
        if (finish_reason_out && !*finish_reason_out) { // Fallback for finishReason
            cJSON *prompt_feedback = cJSON_GetObjectItemCaseSensitive(root, "promptFeedback");
            if (prompt_feedback) {
                cJSON *reason_item = cJSON_GetObjectItemCaseSensitive(prompt_feedback, "finishReason");
                if (cJSON_IsString(reason_item) && reason_item->valuestring) {
                    *finish_reason_out = dp_internal_strdup(reason_item->valuestring);
                }
            }
        }
    } else if (provider == DP_PROVIDER_ANTHROPIC) {
        // Anthropic's non-streaming response structure: top-level 'content' is an array of blocks
        item_array = cJSON_GetObjectItemCaseSensitive(root, "content");
        if(cJSON_IsArray(item_array) && cJSON_GetArraySize(item_array) > 0) {
            cJSON* content_block = cJSON_GetArrayItem(item_array, 0); // Assuming first block is text
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

// Generic text-token streaming callback
static size_t streaming_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    // ... (Existing implementation for OpenAI and Gemini (SSE), now needs Anthropic text extraction) ...
    // For Anthropic, if this generic callback is used, we need to parse its specific SSE
    // and extract only text from "content_block_delta" with "text_delta".

    size_t realsize = size * nmemb;
    stream_processor_t* processor = (stream_processor_t*)userp;

    if (processor->stop_streaming_signal) return 0;

    size_t needed_capacity = processor->buffer_size + realsize + 1;
    if (processor->buffer_capacity < needed_capacity) {
        size_t new_capacity = needed_capacity > processor->buffer_capacity * 2 ? needed_capacity : processor->buffer_capacity * 2;
        if (new_capacity < 1024) new_capacity = 1024;
        char* new_buf = realloc(processor->buffer, new_capacity);
        if (!new_buf) { /* error handling */ return 0;}
        processor->buffer = new_buf;
        processor->buffer_capacity = new_capacity;
    }
    memcpy(processor->buffer + processor->buffer_size, contents, realsize);
    processor->buffer_size += realsize;
    processor->buffer[processor->buffer_size] = '\0';

    char* current_event_start = processor->buffer;
    size_t remaining_in_buffer = processor->buffer_size;

    while (true) {
        if (processor->stop_streaming_signal) break;

        char* event_end_lf = strstr(current_event_start, "\n\n");
        char* event_end_crlf = strstr(current_event_start, "\r\n\r\n");
        char* event_end = NULL;
        size_t separator_len = 0;

        if (event_end_lf && event_end_crlf) {
            event_end = (event_end_lf < event_end_crlf) ? event_end_lf : event_end_crlf;
        } else if (event_end_lf) {
            event_end = event_end_lf;
        } else {
            event_end = event_end_crlf;
        }

        if (event_end == event_end_lf && event_end_lf) separator_len = 2;
        else if (event_end == event_end_crlf && event_end_crlf) separator_len = 4;

        if (!event_end) break;

        size_t event_len = event_end - current_event_start;
        char* event_data_segment = malloc(event_len + 1);
        if (!event_data_segment) { /* error handling */ processor->stop_streaming_signal = true; break; }
        strncpy(event_data_segment, current_event_start, event_len);
        event_data_segment[event_len] = '\0';

        current_event_start = event_end + separator_len;
        remaining_in_buffer -= (event_len + separator_len);

        char* line = event_data_segment;
        char* extracted_token_str = NULL;
        bool is_final_for_this_event = false;
        char* sse_event_type_str = NULL; // For Anthropic

        while (line && *line) {
            char* next_line = strchr(line, '\n');
            if (next_line) {
                *next_line = '\0';
                if ((next_line > line) && (*(next_line - 1) == '\r')) {
                    *(next_line - 1) = '\0';
                }
            }

            if (processor->provider == DP_PROVIDER_ANTHROPIC && strncmp(line, "event: ", 7) == 0) {
                free(sse_event_type_str); // Free previous if any
                sse_event_type_str = dp_internal_strdup(line + 7);
            } else if (strncmp(line, "data: ", 6) == 0) {
                char* json_str = line + 6;

                if (processor->provider == DP_PROVIDER_OPENAI_COMPATIBLE && strcmp(json_str, "[DONE]") == 0) {
                    is_final_for_this_event = true;
                    if (!processor->finish_reason_capture) processor->finish_reason_capture = dp_internal_strdup("done_marker");
                    break;
                }

                cJSON *json_chunk = cJSON_Parse(json_str);
                if (json_chunk) {
                    if (processor->provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
                        // ... (OpenAI parsing - unchanged) ...
                        cJSON *choices = cJSON_GetObjectItemCaseSensitive(json_chunk, "choices");
                        if (cJSON_IsArray(choices) && cJSON_GetArraySize(choices) > 0) {
                            cJSON *choice = cJSON_GetArrayItem(choices, 0);
                            if(choice) {
                                cJSON *delta = cJSON_GetObjectItemCaseSensitive(choice, "delta");
                                if (delta) {
                                    cJSON *content = cJSON_GetObjectItemCaseSensitive(delta, "content");
                                    if (cJSON_IsString(content) && content->valuestring && strlen(content->valuestring) > 0) {
                                        extracted_token_str = dp_internal_strdup(content->valuestring);
                                    }
                                }
                                if (!processor->finish_reason_capture) {
                                    cJSON *reason = cJSON_GetObjectItemCaseSensitive(choice, "finish_reason");
                                    if (cJSON_IsString(reason) && reason->valuestring) {
                                        processor->finish_reason_capture = dp_internal_strdup(reason->valuestring);
                                        is_final_for_this_event = true;
                                    }
                                }
                            }
                        }
                    } else if (processor->provider == DP_PROVIDER_GOOGLE_GEMINI) {
                        // ... (Gemini parsing - unchanged) ...
                        cJSON *candidates = cJSON_GetObjectItemCaseSensitive(json_chunk, "candidates");
                        if (cJSON_IsArray(candidates) && cJSON_GetArraySize(candidates) > 0) {
                            cJSON *candidate = cJSON_GetArrayItem(candidates, 0);
                            if (candidate) {
                                cJSON *content = cJSON_GetObjectItemCaseSensitive(candidate, "content");
                                if (content) {
                                    cJSON *parts = cJSON_GetObjectItemCaseSensitive(content, "parts");
                                    if (cJSON_IsArray(parts)) {
                                        cJSON* part_item_iter = NULL;
                                        char* current_event_accumulated_text = NULL;
                                        cJSON_ArrayForEach(part_item_iter, parts) {
                                            if(part_item_iter){
                                                cJSON *text = cJSON_GetObjectItemCaseSensitive(part_item_iter, "text");
                                                if (cJSON_IsString(text) && text->valuestring) {
                                                    if (strlen(text->valuestring) > 0) {
                                                        if (!current_event_accumulated_text) {
                                                            current_event_accumulated_text = dp_internal_strdup(text->valuestring);
                                                        } else {
                                                            char* old_text = current_event_accumulated_text;
                                                            size_t new_len = strlen(old_text) + strlen(text->valuestring);
                                                            current_event_accumulated_text = malloc(new_len + 1);
                                                            if (current_event_accumulated_text) {
                                                                strcpy(current_event_accumulated_text, old_text);
                                                                strcat(current_event_accumulated_text, text->valuestring);
                                                                free(old_text);
                                                            } else {
                                                                current_event_accumulated_text = old_text;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        if (current_event_accumulated_text) {
                                            free(extracted_token_str);
                                            extracted_token_str = current_event_accumulated_text;
                                        }
                                    }
                                }
                                cJSON *reason_cand = cJSON_GetObjectItemCaseSensitive(candidate, "finishReason");
                                if (cJSON_IsString(reason_cand) && reason_cand->valuestring) {
                                    if (!processor->finish_reason_capture) processor->finish_reason_capture = dp_internal_strdup(reason_cand->valuestring);
                                    is_final_for_this_event = true;
                                }
                            }
                        }
                        cJSON *prompt_feedback = cJSON_GetObjectItemCaseSensitive(json_chunk, "promptFeedback");
                        if (prompt_feedback) {
                            cJSON *reason_pf = cJSON_GetObjectItemCaseSensitive(prompt_feedback, "finishReason");
                            if (cJSON_IsString(reason_pf) && reason_pf->valuestring) {
                                if (!processor->finish_reason_capture) processor->finish_reason_capture = dp_internal_strdup(reason_pf->valuestring);
                                is_final_for_this_event = true;
                            }
                        }
                    } else if (processor->provider == DP_PROVIDER_ANTHROPIC) {
                        if (sse_event_type_str && strcmp(sse_event_type_str, "content_block_delta") == 0) {
                            cJSON *delta = cJSON_GetObjectItemCaseSensitive(json_chunk, "delta");
                            if (delta) {
                                cJSON *type = cJSON_GetObjectItemCaseSensitive(delta, "type");
                                if (cJSON_IsString(type) && strcmp(type->valuestring, "text_delta") == 0) {
                                    cJSON *text = cJSON_GetObjectItemCaseSensitive(delta, "text");
                                    if (cJSON_IsString(text) && text->valuestring && strlen(text->valuestring) > 0) {
                                         extracted_token_str = dp_internal_strdup(text->valuestring);
                                    }
                                }
                            }
                        } else if (sse_event_type_str && strcmp(sse_event_type_str, "message_delta") == 0) {
                             if (!processor->finish_reason_capture) {
                                cJSON *stop_reason = cJSON_GetObjectItemCaseSensitive(json_chunk, "stop_reason");
                                if(cJSON_IsString(stop_reason) && stop_reason->valuestring){
                                    processor->finish_reason_capture = dp_internal_strdup(stop_reason->valuestring);
                                }
                             }
                        } else if (sse_event_type_str && strcmp(sse_event_type_str, "message_stop") == 0) {
                            is_final_for_this_event = true;
                            // finish_reason should have been captured from message_delta or earlier content_block_delta
                        } else if (sse_event_type_str && strcmp(sse_event_type_str, "error") == 0) {
                            cJSON* error_obj = cJSON_GetObjectItemCaseSensitive(json_chunk, "error");
                            if(error_obj) {
                                cJSON* err_type = cJSON_GetObjectItemCaseSensitive(error_obj, "type");
                                cJSON* err_msg = cJSON_GetObjectItemCaseSensitive(error_obj, "message");
                                if(cJSON_IsString(err_type) && cJSON_IsString(err_msg)){
                                     char* temp_err;
                                     asprintf(&temp_err, "Anthropic Stream Error (%s): %s", err_type->valuestring, err_msg->valuestring);
                                     if(!processor->accumulated_error_during_stream) processor->accumulated_error_during_stream = temp_err; else free(temp_err);
                                }
                            }
                            is_final_for_this_event = true; // Error usually means end of stream
                        }
                    } // End provider check
                    cJSON_Delete(json_chunk);
                } // End if json_chunk
            } // End if "data: "
            if (next_line) line = next_line + 1 ; else break;
        } // End while lines in event_data_segment
        free(sse_event_type_str); sse_event_type_str = NULL;
        free(event_data_segment);

        if (extracted_token_str) {
            if (processor->user_callback(extracted_token_str, processor->user_data, is_final_for_this_event, NULL) != 0) {
                processor->stop_streaming_signal = true;
            }
            free(extracted_token_str);
            extracted_token_str = NULL;
        } else if (is_final_for_this_event) {
            if (processor->user_callback(NULL, processor->user_data, true, processor->accumulated_error_during_stream) != 0) {
                processor->stop_streaming_signal = true;
            }
        }
        if (is_final_for_this_event) processor->stop_streaming_signal = true;
    }

    if (remaining_in_buffer > 0 && current_event_start < processor->buffer + processor->buffer_size) {
        memmove(processor->buffer, current_event_start, remaining_in_buffer);
    } else if (remaining_in_buffer == 0) { // Cleared buffer
        processor->buffer_size = 0;
    }
    // If remaining_in_buffer > 0 and current_event_start == processor->buffer, it means nothing was processed.
    // This can happen if the buffer doesn't contain a full SSE separator yet.
    // In this case, buffer_size remains as is, waiting for more data.

    processor->buffer_size = remaining_in_buffer; // Update buffer size
    if (processor->buffer_size < processor->buffer_capacity) {
        processor->buffer[processor->buffer_size] = '\0';
    }
    return realsize;
}


// New write callback for Anthropic detailed streaming
static size_t anthropic_detailed_stream_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    anthropic_stream_processor_t* processor = (anthropic_stream_processor_t*)userp;

    if (processor->stop_streaming_signal) return 0;

    size_t needed_capacity = processor->buffer_size + realsize + 1;
    if (processor->buffer_capacity < needed_capacity) {
        size_t new_capacity = needed_capacity > processor->buffer_capacity * 2 ? needed_capacity : processor->buffer_capacity * 2;
        if (new_capacity < 1024) new_capacity = 1024;
        char* new_buf = realloc(processor->buffer, new_capacity);
        if (!new_buf) {
            dp_anthropic_stream_event_t event = { .event_type = DP_ANTHROPIC_EVENT_ERROR, .raw_json_data = "{\"error\":\"Stream buffer memory re-allocation failed\"}" };
            processor->anthropic_user_callback(&event, processor->user_data, "Stream buffer memory re-allocation failed");
            if (!processor->accumulated_error_during_stream) processor->accumulated_error_during_stream = dp_internal_strdup("Stream buffer memory re-allocation failed");
            return 0;
        }
        processor->buffer = new_buf;
        processor->buffer_capacity = new_capacity;
    }
    memcpy(processor->buffer + processor->buffer_size, contents, realsize);
    processor->buffer_size += realsize;
    processor->buffer[processor->buffer_size] = '\0';

    char* current_event_start = processor->buffer;
    size_t remaining_in_buffer = processor->buffer_size;

    while (true) {
        if (processor->stop_streaming_signal) break;

        char* event_end_lf = strstr(current_event_start, "\n\n");
        char* event_end_crlf = strstr(current_event_start, "\r\n\r\n");
        char* event_end = NULL;
        size_t separator_len = 0;

        if (event_end_lf && event_end_crlf) {
            event_end = (event_end_lf < event_end_crlf) ? event_end_lf : event_end_crlf;
        } else if (event_end_lf) {
            event_end = event_end_lf;
        } else {
            event_end = event_end_crlf;
        }

        if (event_end == event_end_lf && event_end_lf) separator_len = 2;
        else if (event_end == event_end_crlf && event_end_crlf) separator_len = 4;

        if (!event_end) break;

        size_t event_len = event_end - current_event_start;
        char* event_data_segment = malloc(event_len + 1);
        if (!event_data_segment) {
            dp_anthropic_stream_event_t event = { .event_type = DP_ANTHROPIC_EVENT_ERROR, .raw_json_data = "{\"error\":\"Event segment memory allocation failed\"}" };
            processor->anthropic_user_callback(&event, processor->user_data, "Event segment memory allocation failed");
            if(!processor->accumulated_error_during_stream) processor->accumulated_error_during_stream = dp_internal_strdup("Event segment memory allocation failed");
            processor->stop_streaming_signal = true; break;
        }
        strncpy(event_data_segment, current_event_start, event_len);
        event_data_segment[event_len] = '\0';

        current_event_start = event_end + separator_len;
        remaining_in_buffer -= (event_len + separator_len);

        char* line = event_data_segment;
        dp_anthropic_stream_event_t current_api_event = { .event_type = DP_ANTHROPIC_EVENT_UNKNOWN, .raw_json_data = NULL};
        char* temp_event_type_str = NULL;
        char* temp_json_data_str = NULL;

        while (line && *line) {
            char* next_line = strchr(line, '\n');
            if (next_line) {
                *next_line = '\0';
                if ((next_line > line) && (*(next_line - 1) == '\r')) {
                    *(next_line - 1) = '\0';
                }
            }

            if (strncmp(line, "event: ", 7) == 0) {
                free(temp_event_type_str); // free previous type if accumulating multi-line event (should not happen with Anthropic SSE usually)
                temp_event_type_str = dp_internal_strdup(line + 7);
            } else if (strncmp(line, "data: ", 6) == 0) {
                free(temp_json_data_str); // free previous data if accumulating
                temp_json_data_str = dp_internal_strdup(line + 6);
            }
            if (next_line) line = next_line + 1 ; else break;
        }

        if (temp_event_type_str) {
            if (strcmp(temp_event_type_str, "message_start") == 0) current_api_event.event_type = DP_ANTHROPIC_EVENT_MESSAGE_START;
            else if (strcmp(temp_event_type_str, "content_block_start") == 0) current_api_event.event_type = DP_ANTHROPIC_EVENT_CONTENT_BLOCK_START;
            else if (strcmp(temp_event_type_str, "ping") == 0) current_api_event.event_type = DP_ANTHROPIC_EVENT_PING;
            else if (strcmp(temp_event_type_str, "content_block_delta") == 0) current_api_event.event_type = DP_ANTHROPIC_EVENT_CONTENT_BLOCK_DELTA;
            else if (strcmp(temp_event_type_str, "content_block_stop") == 0) current_api_event.event_type = DP_ANTHROPIC_EVENT_CONTENT_BLOCK_STOP;
            else if (strcmp(temp_event_type_str, "message_delta") == 0) current_api_event.event_type = DP_ANTHROPIC_EVENT_MESSAGE_DELTA;
            else if (strcmp(temp_event_type_str, "message_stop") == 0) current_api_event.event_type = DP_ANTHROPIC_EVENT_MESSAGE_STOP;
            else if (strcmp(temp_event_type_str, "error") == 0) current_api_event.event_type = DP_ANTHROPIC_EVENT_ERROR;
            else current_api_event.event_type = DP_ANTHROPIC_EVENT_UNKNOWN;
            free(temp_event_type_str);
        }
        current_api_event.raw_json_data = temp_json_data_str; // temp_json_data_str is already a strdup or NULL

        // Capture overall finish reason from message_stop or error event
        if (current_api_event.event_type == DP_ANTHROPIC_EVENT_MESSAGE_STOP || current_api_event.event_type == DP_ANTHROPIC_EVENT_ERROR) {
             if (!processor->finish_reason_capture && current_api_event.raw_json_data) {
                cJSON* data_json = cJSON_Parse(current_api_event.raw_json_data);
                if(data_json) {
                    if (current_api_event.event_type == DP_ANTHROPIC_EVENT_MESSAGE_STOP) {
                        // Anthropic's message_stop event itself might not contain a stop_reason field in its 'data'.
                        // The stop_reason is usually in the 'message_delta' that precedes message_stop, or on content blocks.
                        // For simplicity, we'll mark it. A more detailed parser could extract from message_delta.
                        processor->finish_reason_capture = dp_internal_strdup("message_stop_event");
                    } else { // DP_ANTHROPIC_EVENT_ERROR
                        cJSON* error_obj = cJSON_GetObjectItemCaseSensitive(data_json, "error");
                        if(error_obj) {
                            cJSON* type_item = cJSON_GetObjectItemCaseSensitive(error_obj, "type");
                            if(cJSON_IsString(type_item)) {
                                processor->finish_reason_capture = dp_internal_strdup(type_item->valuestring);
                            } else {
                                processor->finish_reason_capture = dp_internal_strdup("error_event");
                            }
                        }
                    }
                    cJSON_Delete(data_json);
                }
             }
             processor->stop_streaming_signal = true; // Stop after final event
        }

        if (processor->anthropic_user_callback(&current_api_event, processor->user_data, NULL) != 0) {
            processor->stop_streaming_signal = true;
        }
        free(temp_json_data_str); // Free the strdup'd JSON data after callback
        free(event_data_segment);
    }

    if (remaining_in_buffer > 0 && current_event_start < processor->buffer + processor->buffer_size) {
        memmove(processor->buffer, current_event_start, remaining_in_buffer);
    } else if (remaining_in_buffer == 0) {
        processor->buffer_size = 0;
    }
    processor->buffer_size = remaining_in_buffer;
    if (processor->buffer_size < processor->buffer_capacity) {
        processor->buffer[processor->buffer_size] = '\0';
    }
    return realsize;
}


int dp_perform_completion(dp_context_t* context, const dp_request_config_t* request_config, dp_response_t* response) {
    if (!context || !request_config || !response) {
        if (response) response->error_message = dp_internal_strdup("Invalid arguments to dp_perform_completion.");
        return -1;
    }
    if (request_config->stream && context->provider != DP_PROVIDER_ANTHROPIC) { // Anthropic handles stream via this func too, but has specific one.
        if (response) response->error_message = dp_internal_strdup("dp_perform_completion called with stream=true. Use dp_perform_streaming_completion or dp_perform_anthropic_streaming_completion.");
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
        headers = curl_slist_append(headers, "anthropic-version: 2023-06-01"); // Common Anthropic version
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
    curl_easy_setopt(curl, CURLOPT_USERAGENT, DISASTERPARTY_USER_AGENT);

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
                    if (error_obj) { // General error structure
                        cJSON* msg_item = cJSON_GetObjectItemCaseSensitive(error_obj, "message");
                        if (cJSON_IsString(msg_item) && msg_item->valuestring) {
                             asprintf(&response->error_message, "API error (HTTP %ld): %s", response->http_status_code, msg_item->valuestring);
                        }
                    } else { // Anthropic specific error structure might be at top level
                         cJSON* type_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_root, "type");
                         cJSON* msg_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_root, "message"); // Anthropic error often has message at root of error obj
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
                if(error_obj){ // General
                    cJSON* msg_item = cJSON_GetObjectItemCaseSensitive(error_obj, "message");
                    if(cJSON_IsString(msg_item) && msg_item->valuestring){
                        api_err_detail = msg_item->valuestring;
                    }
                } else { // Anthropic specific
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


int dp_perform_streaming_completion(dp_context_t* context, const dp_request_config_t* request_config,
                                    dp_stream_callback_t callback, void* user_data, dp_response_t* response) {
    if (!context || !request_config || !callback || !response) {
        if (response) response->error_message = dp_internal_strdup("Invalid arguments to dp_perform_streaming_completion.");
        return -1;
    }
    // This function is for generic text streaming. If Anthropic is chosen, it will adapt.
    // For detailed Anthropic events, dp_perform_anthropic_streaming_completion should be used.
    if (context->provider == DP_PROVIDER_OPENAI_COMPATIBLE && !request_config->stream) {
         if (response) response->error_message = dp_internal_strdup("dp_perform_streaming_completion for OpenAI requires stream=true in config.");
        return -1;
    }
     if (context->provider == DP_PROVIDER_ANTHROPIC && !request_config->stream) {
         if (response) response->error_message = dp_internal_strdup("dp_perform_streaming_completion for Anthropic requires stream=true in config.");
        return -1;
    }

    memset(response, 0, sizeof(dp_response_t));

    CURL* curl = curl_easy_init();
    if (!curl) {
        response->error_message = dp_internal_strdup("curl_easy_init() failed for Disaster Party streaming.");
        return -1;
    }

    stream_processor_t processor = {0}; // Generic stream processor
    processor.user_callback = callback;
    processor.user_data = user_data;
    processor.provider = context->provider;
    processor.buffer_capacity = 8192;
    processor.buffer = malloc(processor.buffer_capacity);
    if (!processor.buffer) {
        response->error_message = dp_internal_strdup("Stream processor buffer alloc failed.");
        curl_easy_cleanup(curl); return -1;
    }
    processor.buffer[0] = '\0';

    char* json_payload_str = NULL;
    if (context->provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
        json_payload_str = build_openai_json_payload_with_cjson(request_config);
    } else if (context->provider == DP_PROVIDER_GOOGLE_GEMINI) {
        json_payload_str = build_gemini_json_payload_with_cjson(request_config);
    } else if (context->provider == DP_PROVIDER_ANTHROPIC) {
        json_payload_str = build_anthropic_json_payload_with_cjson(request_config);
    }

    if (!json_payload_str) {
        response->error_message = dp_internal_strdup("Payload build failed for streaming.");
        free(processor.buffer); curl_easy_cleanup(curl); return -1;
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
        snprintf(url, sizeof(url), "%s/models/%s:streamGenerateContent?key=%s&alt=sse",
                 context->api_base_url, request_config->model, context->api_key);
    } else if (context->provider == DP_PROVIDER_ANTHROPIC) {
        snprintf(url, sizeof(url), "%s/messages", context->api_base_url);
        char api_key_header[512];
        snprintf(api_key_header, sizeof(api_key_header), "x-api-key: %s", context->api_key);
        headers = curl_slist_append(headers, api_key_header);
        headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
    }


    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload_str);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, streaming_write_callback); // Generic callback handler
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&processor);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, DISASTERPARTY_USER_AGENT);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_status_code);

    if (!processor.stop_streaming_signal) {
        // ... (rest of error handling as in previous version) ...
        const char* final_stream_error = processor.accumulated_error_during_stream;
        if (res != CURLE_OK && !final_stream_error) {
            final_stream_error = curl_easy_strerror(res);
        } else if ((response->http_status_code < 200 || response->http_status_code >= 300) && !final_stream_error) {
            cJSON* error_json = cJSON_Parse(processor.buffer);
            if (error_json) {
                cJSON* error_obj = cJSON_GetObjectItemCaseSensitive(error_json, "error");
                 if (error_obj) { // General error structure
                    cJSON* msg_item = cJSON_GetObjectItemCaseSensitive(error_obj, "message");
                    if (cJSON_IsString(msg_item) && msg_item->valuestring) {
                        final_stream_error = msg_item->valuestring;
                        if (response->error_message == NULL) {
                           response->error_message = dp_internal_strdup(final_stream_error);
                        }
                    }
                } else { // Anthropic specific error structure might be at top level
                     cJSON* type_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_json, "type");
                     cJSON* msg_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_json, "message");
                     if(cJSON_IsString(type_item_anthropic) && strcmp(type_item_anthropic->valuestring, "error") == 0 &&
                        cJSON_IsString(msg_item_anthropic) && msg_item_anthropic->valuestring) {
                        final_stream_error = msg_item_anthropic->valuestring;
                        if (response->error_message == NULL) {
                           response->error_message = dp_internal_strdup(final_stream_error);
                        }
                     }
                }
                cJSON_Delete(error_json);
            }
            if (!final_stream_error && processor.buffer_size > 0) {
                 final_stream_error = processor.buffer;
            } else if (!final_stream_error) {
                final_stream_error = "HTTP error occurred during stream";
            }
        }
        processor.user_callback(NULL, processor.user_data, true, final_stream_error);
    }

    if (processor.finish_reason_capture) {
        response->finish_reason = processor.finish_reason_capture;
    } else if (res == CURLE_OK && response->http_status_code >= 200 && response->http_status_code < 300 && !processor.accumulated_error_during_stream && !response->finish_reason) {
        response->finish_reason = dp_internal_strdup("completed");
    }

    if (res != CURLE_OK && !response->error_message) {
        asprintf(&response->error_message, "curl_easy_perform() failed: %s", curl_easy_strerror(res));
    } else if ((response->http_status_code < 200 || response->http_status_code >= 300) && !response->error_message) {
         // ... (rest of error message population as in previous version) ...
         char* temp_err_msg = NULL;
         if (processor.buffer_size > 0) {
            cJSON* error_json = cJSON_Parse(processor.buffer);
            if (error_json) {
                cJSON* error_obj = cJSON_GetObjectItemCaseSensitive(error_json, "error");
                if (error_obj) {
                    cJSON* msg_item = cJSON_GetObjectItemCaseSensitive(error_obj, "message");
                    if (cJSON_IsString(msg_item) && msg_item->valuestring) {
                        asprintf(&temp_err_msg, "HTTP error %ld: %s", response->http_status_code, msg_item->valuestring);
                    }
                } else {
                     cJSON* type_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_json, "type");
                     cJSON* msg_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_json, "message");
                     if(cJSON_IsString(type_item_anthropic) && strcmp(type_item_anthropic->valuestring, "error") == 0 &&
                        cJSON_IsString(msg_item_anthropic) && msg_item_anthropic->valuestring) {
                        asprintf(&temp_err_msg, "HTTP error %ld: %s", response->http_status_code, msg_item_anthropic->valuestring);
                     }
                }
                cJSON_Delete(error_json);
            }
         }
         if (temp_err_msg) {
            response->error_message = temp_err_msg;
         } else if (processor.buffer_size > 0) {
            asprintf(&response->error_message, "HTTP error %ld. Body hint: %.200s", response->http_status_code, processor.buffer);
         } else {
            asprintf(&response->error_message, "HTTP error %ld (empty body)", response->http_status_code);
         }
    }

    if (processor.accumulated_error_during_stream) {
        if (response->error_message) {
             char* combined_error;
             if (strstr(response->error_message, processor.accumulated_error_during_stream) == NULL) {
                asprintf(&combined_error, "%s; Stream processing error: %s", response->error_message, processor.accumulated_error_during_stream);
                free(response->error_message);
                response->error_message = combined_error;
             }
             free(processor.accumulated_error_during_stream);
        } else {
            response->error_message = processor.accumulated_error_during_stream;
        }
    }

    free(json_payload_str);
    free(processor.buffer);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response->error_message ? -1 : 0;
}

// New function for Anthropic detailed streaming
int dp_perform_anthropic_streaming_completion(dp_context_t* context,
                                              const dp_request_config_t* request_config,
                                              dp_anthropic_stream_callback_t anthropic_callback,
                                              void* user_data,
                                              dp_response_t* response) {
    if (!context || !request_config || !anthropic_callback || !response) {
        if (response) response->error_message = dp_internal_strdup("Invalid arguments to dp_perform_anthropic_streaming_completion.");
        return -1;
    }
    if (context->provider != DP_PROVIDER_ANTHROPIC) {
        if (response) response->error_message = dp_internal_strdup("dp_perform_anthropic_streaming_completion called with non-Anthropic provider.");
        return -1;
    }
    if (!request_config->stream) {
         if (response) response->error_message = dp_internal_strdup("dp_perform_anthropic_streaming_completion requires stream=true in config.");
        return -1;
    }

    memset(response, 0, sizeof(dp_response_t));

    CURL* curl = curl_easy_init();
    if (!curl) {
        response->error_message = dp_internal_strdup("curl_easy_init() failed for Anthropic streaming.");
        return -1;
    }

    anthropic_stream_processor_t processor = {0}; // Anthropic-specific processor
    processor.anthropic_user_callback = anthropic_callback;
    processor.user_data = user_data;
    processor.buffer_capacity = 8192;
    processor.buffer = malloc(processor.buffer_capacity);
    if (!processor.buffer) {
        response->error_message = dp_internal_strdup("Anthropic stream processor buffer alloc failed.");
        curl_easy_cleanup(curl); return -1;
    }
    processor.buffer[0] = '\0';

    char* json_payload_str = build_anthropic_json_payload_with_cjson(request_config);
    if (!json_payload_str) {
        response->error_message = dp_internal_strdup("Payload build failed for Anthropic streaming.");
        free(processor.buffer); curl_easy_cleanup(curl); return -1;
    }

    char url[1024];
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    snprintf(url, sizeof(url), "%s/messages", context->api_base_url);
    char api_key_header[512];
    snprintf(api_key_header, sizeof(api_key_header), "x-api-key: %s", context->api_key);
    headers = curl_slist_append(headers, api_key_header);
    headers = curl_slist_append(headers, "anthropic-version: 2023-06-01"); // Or a more recent version if available/required

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload_str);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, anthropic_detailed_stream_write_callback); // Use new callback
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&processor);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, DISASTERPARTY_USER_AGENT);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_status_code);

    if (!processor.stop_streaming_signal) {
        const char* final_stream_error = processor.accumulated_error_during_stream;
         dp_anthropic_stream_event_t final_event = { .event_type = DP_ANTHROPIC_EVENT_UNKNOWN, .raw_json_data = NULL};

        if (res != CURLE_OK && !final_stream_error) {
            final_stream_error = curl_easy_strerror(res);
            final_event.event_type = DP_ANTHROPIC_EVENT_ERROR; // Treat curl error as a stream error event
            final_event.raw_json_data = final_stream_error; // Not JSON, but best effort
        } else if ((response->http_status_code < 200 || response->http_status_code >= 300) && !final_stream_error) {
            final_event.event_type = DP_ANTHROPIC_EVENT_ERROR;
            final_event.raw_json_data = processor.buffer_size > 0 ? processor.buffer : "{\"error\":\"HTTP error occurred during stream\"}";
            final_stream_error = processor.buffer_size > 0 ? processor.buffer : "HTTP error occurred during stream";
        } else if (!final_stream_error) { // Clean end, but ensure a final signal if not already done by message_stop
            final_event.event_type = DP_ANTHROPIC_EVENT_MESSAGE_STOP; // Simulate if not caught explicitly
        }
        // Call user callback one last time to signal end or error.
        // Only if not already stopped by user or message_stop.
        // The stop_streaming_signal helps prevent double-calling for message_stop.
        if (final_event.event_type != DP_ANTHROPIC_EVENT_UNKNOWN || final_stream_error) {
            processor.anthropic_user_callback(&final_event, processor.user_data, final_stream_error);
        }
    }

    if (processor.finish_reason_capture) {
        response->finish_reason = processor.finish_reason_capture;
    } else if (res == CURLE_OK && response->http_status_code >= 200 && response->http_status_code < 300 && !processor.accumulated_error_during_stream && !response->finish_reason) {
        response->finish_reason = dp_internal_strdup("completed"); // Or derive from last Anthropic event if possible
    }

    // Populate response->error_message for overall function status
    // (Logic similar to dp_perform_streaming_completion's end)
    if (res != CURLE_OK && !response->error_message) {
        asprintf(&response->error_message, "curl_easy_perform() failed: %s", curl_easy_strerror(res));
    } else if ((response->http_status_code < 200 || response->http_status_code >= 300) && !response->error_message) {
         // ... (error message population as in generic streaming) ...
    }
    if (processor.accumulated_error_during_stream) {
        if (response->error_message) { /* combine */ } else { response->error_message = processor.accumulated_error_during_stream; }
    }


    free(json_payload_str);
    free(processor.buffer);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response->error_message ? -1 : 0;
}



// Implementation for dp_list_models
int dp_list_models(dp_context_t* context, dp_model_list_t** model_list_out) {
    if (!context || !model_list_out) {
        if (model_list_out) *model_list_out = NULL;
        return -1;
    }

    *model_list_out = calloc(1, sizeof(dp_model_list_t));
    if (!*model_list_out) {
        return -1;
    }
    (*model_list_out)->models = NULL;
    (*model_list_out)->count = 0;
    (*model_list_out)->error_message = NULL;
    (*model_list_out)->http_status_code = 0;

    CURL* curl = curl_easy_init();
    if (!curl) {
        (*model_list_out)->error_message = dp_internal_strdup("curl_easy_init() failed for list_models.");
        return -1;
    }

    char url[1024];
    struct curl_slist* headers = NULL;
    memory_struct_t chunk_mem = { .memory = malloc(1), .size = 0 };
    if (!chunk_mem.memory) {
        (*model_list_out)->error_message = dp_internal_strdup("Memory allocation for list_models response chunk failed.");
        curl_easy_cleanup(curl);
        return -1;
    }
    chunk_mem.memory[0] = '\0';

    if (context->provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
        snprintf(url, sizeof(url), "%s/models", context->api_base_url);
        char auth_header[512];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", context->api_key);
        headers = curl_slist_append(headers, auth_header);
    } else if (context->provider == DP_PROVIDER_GOOGLE_GEMINI) {
        snprintf(url, sizeof(url), "%s/models?key=%s", context->api_base_url, context->api_key);
    } else if (context->provider == DP_PROVIDER_ANTHROPIC) {
        snprintf(url, sizeof(url), "%s/models", context->api_base_url); // Anthropic also uses /models
        char api_key_header[512];
        snprintf(api_key_header, sizeof(api_key_header), "x-api-key: %s", context->api_key);
        headers = curl_slist_append(headers, api_key_header);
        headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
    } else {
        (*model_list_out)->error_message = dp_internal_strdup("Unsupported provider for list_models.");
        free(chunk_mem.memory);
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk_mem);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, DISASTERPARTY_USER_AGENT);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(*model_list_out)->http_status_code);

    int return_code = 0;

    if (res != CURLE_OK) {
        asprintf(&(*model_list_out)->error_message, "curl_easy_perform() failed for list_models: %s (HTTP status: %ld)",
                 curl_easy_strerror(res), (*model_list_out)->http_status_code);
        return_code = -1;
    } else {
        if ((*model_list_out)->http_status_code >= 200 && (*model_list_out)->http_status_code < 300) {
            cJSON *root = cJSON_Parse(chunk_mem.memory);
            if (!root) {
                asprintf(&(*model_list_out)->error_message, "Failed to parse JSON response for list_models. Body: %.200s...", chunk_mem.memory ? chunk_mem.memory : "(empty)");
                return_code = -1;
            } else {
                cJSON *data_array = NULL;
                // OpenAI and Anthropic use "data", Gemini uses "models"
                if (context->provider == DP_PROVIDER_OPENAI_COMPATIBLE || context->provider == DP_PROVIDER_ANTHROPIC) {
                    data_array = cJSON_GetObjectItemCaseSensitive(root, "data");
                } else if (context->provider == DP_PROVIDER_GOOGLE_GEMINI) {
                    data_array = cJSON_GetObjectItemCaseSensitive(root, "models");
                }

                if (cJSON_IsArray(data_array)) {
                    int array_size = cJSON_GetArraySize(data_array);
                    (*model_list_out)->models = calloc(array_size, sizeof(dp_model_info_t));
                    if (!(*model_list_out)->models) {
                        (*model_list_out)->error_message = dp_internal_strdup("Failed to allocate memory for model info array.");
                        return_code = -1;
                    } else {
                        (*model_list_out)->count = array_size;
                        cJSON *model_json = NULL;
                        int current_model_idx = 0;
                        cJSON_ArrayForEach(model_json, data_array) {
                            dp_model_info_t *info = &((*model_list_out)->models[current_model_idx]);
                            memset(info, 0, sizeof(dp_model_info_t));

                            cJSON *id_item = NULL;
                            if (context->provider == DP_PROVIDER_OPENAI_COMPATIBLE || context->provider == DP_PROVIDER_ANTHROPIC) {
                                id_item = cJSON_GetObjectItemCaseSensitive(model_json, "id");
                            } else { // Gemini
                                id_item = cJSON_GetObjectItemCaseSensitive(model_json, "name");
                            }
                            if (cJSON_IsString(id_item) && id_item->valuestring) {
                                info->model_id = dp_internal_strdup(id_item->valuestring);
                            }

                            cJSON *display_name_item = cJSON_GetObjectItemCaseSensitive(model_json, "displayName"); // Gemini specific
                             if (!display_name_item && context->provider == DP_PROVIDER_ANTHROPIC) { // Anthropic specific
                                display_name_item = cJSON_GetObjectItemCaseSensitive(model_json, "display_name");
                            }
                            if (cJSON_IsString(display_name_item) && display_name_item->valuestring) {
                                info->display_name = dp_internal_strdup(display_name_item->valuestring);
                            }

                            cJSON *version_item = cJSON_GetObjectItemCaseSensitive(model_json, "version"); // Gemini specific
                            if (cJSON_IsString(version_item) && version_item->valuestring) {
                                info->version = dp_internal_strdup(version_item->valuestring);
                            }

                            cJSON *description_item = cJSON_GetObjectItemCaseSensitive(model_json, "description"); // Gemini specific
                            if (cJSON_IsString(description_item) && description_item->valuestring) {
                                info->description = dp_internal_strdup(description_item->valuestring);
                            }

                            cJSON *input_limit_item = cJSON_GetObjectItemCaseSensitive(model_json, "inputTokenLimit"); // Gemini specific
                            if(cJSON_IsNumber(input_limit_item)) {
                                info->input_token_limit = (long)input_limit_item->valuedouble;
                            }

                            cJSON *output_limit_item = cJSON_GetObjectItemCaseSensitive(model_json, "outputTokenLimit"); // Gemini specific
                             if(cJSON_IsNumber(output_limit_item)) {
                                info->output_token_limit = (long)output_limit_item->valuedouble;
                            }
                            current_model_idx++;
                        }
                    }
                } else {
                     asprintf(&(*model_list_out)->error_message, "Expected an array for model listing response. Body: %.200s...", chunk_mem.memory ? chunk_mem.memory : "(empty)");
                    return_code = -1;
                }
                cJSON_Delete(root);
            }
        } else {
            asprintf(&(*model_list_out)->error_message, "list_models HTTP error %ld. Body: %.500s", (*model_list_out)->http_status_code, chunk_mem.memory ? chunk_mem.memory : "(no response body)");
            return_code = -1;
        }
    }

    free(chunk_mem.memory);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (return_code == -1 && (*model_list_out)->models == NULL && (*model_list_out)->error_message == NULL) {
         (*model_list_out)->error_message = dp_internal_strdup("Unknown error in dp_list_models before HTTP response processing.");
    }

    return return_code;
}

void dp_free_model_list(dp_model_list_t* model_list) {
    if (!model_list) return;

    if (model_list->models) {
        for (size_t i = 0; i < model_list->count; ++i) {
            free(model_list->models[i].model_id);
            free(model_list->models[i].display_name);
            free(model_list->models[i].version);
            free(model_list->models[i].description);
        }
        free(model_list->models);
    }
    free(model_list->error_message);
    free(model_list);
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
                }
            }
            free(messages[i].parts);
            messages[i].parts = NULL;
            messages[i].num_parts = 0;
        }
    }
}

static bool dp_message_add_part_internal(dp_message_t* message, dp_content_part_type_t type,
                                   const char* text_content, const char* image_url_content,
                                   const char* mime_type_content, const char* base64_data_content) {
    if (!message) return false;
    dp_content_part_t* new_parts_array = realloc(message->parts, (message->num_parts + 1) * sizeof(dp_content_part_t));
    if (!new_parts_array) return false;
    message->parts = new_parts_array;

    dp_content_part_t* new_part = &message->parts[message->num_parts];
    memset(new_part, 0, sizeof(dp_content_part_t));
    new_part->type = type;
    bool success = false;

    if (type == DP_CONTENT_PART_TEXT && text_content) {
        new_part->text = dp_internal_strdup(text_content);
        success = (new_part->text != NULL);
    } else if (type == DP_CONTENT_PART_IMAGE_URL && image_url_content) {
        new_part->image_url = dp_internal_strdup(image_url_content);
        success = (new_part->image_url != NULL);
    } else if (type == DP_CONTENT_PART_IMAGE_BASE64 && mime_type_content && base64_data_content) {
        new_part->image_base64.mime_type = dp_internal_strdup(mime_type_content);
        new_part->image_base64.data = dp_internal_strdup(base64_data_content);
        success = (new_part->image_base64.mime_type != NULL && new_part->image_base64.data != NULL);
        if (!success) {
            free(new_part->image_base64.mime_type); new_part->image_base64.mime_type = NULL;
            free(new_part->image_base64.data); new_part->image_base64.data = NULL;
        }
    }
    if (success) message->num_parts++;
    else fprintf(stderr, "Failed to allocate memory for Disaster Party message content part.\n");
    return success;
}

bool dp_message_add_text_part(dp_message_t* message, const char* text) {
    return dp_message_add_part_internal(message, DP_CONTENT_PART_TEXT, text, NULL, NULL, NULL);
}
bool dp_message_add_image_url_part(dp_message_t* message, const char* image_url) {
    return dp_message_add_part_internal(message, DP_CONTENT_PART_IMAGE_URL, NULL, image_url, NULL, NULL);
}
bool dp_message_add_base64_image_part(dp_message_t* message, const char* mime_type, const char* base64_data) {
    return dp_message_add_part_internal(message, DP_CONTENT_PART_IMAGE_BASE64, NULL, NULL, mime_type, base64_data);
}
