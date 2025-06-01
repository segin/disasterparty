#define _GNU_SOURCE 
#include "diasterparty.h" // Updated include
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h> 

// Default base URLs
const char* DEFAULT_OPENAI_API_BASE_URL = "https://api.openai.com/v1";
const char* DEFAULT_GEMINI_API_BASE_URL = "https://generativelanguage.googleapis.com/v1beta";
const char* DIASTERPARTY_USER_AGENT = "diasterparty_c/" DIASTERPARTY_VERSION;


// Internal structure for the client context
struct dpt_context_s { // Renamed 
    dpt_provider_type_t provider;
    char* api_key;
    char* api_base_url;
    CURL* curl_handle; 
};

typedef struct {
    char* memory;
    size_t size;
} memory_struct_t;

typedef struct {
    dpt_stream_callback_t user_callback; // Updated type
    void* user_data;
    char* buffer; 
    size_t buffer_size;
    size_t buffer_capacity;
    dpt_provider_type_t provider; // Updated type
    char* finish_reason_capture; 
    bool stop_streaming_signal; 
    char* accumulated_error_during_stream; 
} stream_processor_t;


const char* dpt_get_version(void) {
    return DIASTERPARTY_VERSION;
}

static size_t write_memory_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    memory_struct_t* mem = (memory_struct_t*)userp;
    char* ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL) {
        fprintf(stderr, "write_memory_callback: not enough memory (realloc returned NULL)\n");
        return 0; 
    }
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

static char* duplicate_string(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* new_s = malloc(len);
    if (!new_s) return NULL;
    memcpy(new_s, s, len);
    return new_s;
}

static char* escape_json_string(const char* input) {
    if (!input) return NULL;
    size_t input_len = strlen(input);
    char* escaped_str = malloc(input_len * 6 + 3); 
    if (!escaped_str) return NULL;
    char* p_out = escaped_str;
    *p_out++ = '"';
    for (size_t i = 0; i < input_len; ++i) {
        unsigned char c = input[i];
        switch (c) {
            case '"':  *p_out++ = '\\'; *p_out++ = '"';  break;
            case '\\': *p_out++ = '\\'; *p_out++ = '\\'; break;
            case '\b': *p_out++ = '\\'; *p_out++ = 'b';  break;
            case '\f': *p_out++ = '\\'; *p_out++ = 'f';  break;
            case '\n': *p_out++ = '\\'; *p_out++ = 'n';  break;
            case '\r': *p_out++ = '\\'; *p_out++ = 'r';  break;
            case '\t': *p_out++ = '\\'; *p_out++ = 't';  break;
            default:
                if (c < 32 || c >= 127) { 
                    snprintf(p_out, 7, "\\u%04x", c); p_out += 6; 
                } else { *p_out++ = c; }
                break;
        }
    }
    *p_out++ = '"';
    *p_out = '\0';
    return escaped_str;
}

dpt_context_t* dpt_init_context(dpt_provider_type_t provider,
                              const char* api_key,
                              const char* api_base_url) {
    if (!api_key) {
        fprintf(stderr, "API key is required.\n");
        return NULL;
    }
    dpt_context_t* context = calloc(1, sizeof(dpt_context_t));
    if (!context) {
        perror("Failed to allocate context");
        return NULL;
    }
    context->provider = provider;
    context->api_key = duplicate_string(api_key);
    if (api_base_url) {
        context->api_base_url = duplicate_string(api_base_url);
    } else {
        context->api_base_url = duplicate_string(
            provider == DPT_PROVIDER_OPENAI_COMPATIBLE ? DEFAULT_OPENAI_API_BASE_URL : DEFAULT_GEMINI_API_BASE_URL
        );
    }
    if (!context->api_key || !context->api_base_url) {
        perror("Failed to allocate API key or base URL");
        free(context->api_key); free(context->api_base_url); free(context);
        return NULL;
    }
    context->curl_handle = NULL; 
    return context;
}

void dpt_destroy_context(dpt_context_t* context) {
    if (!context) return;
    if (context->curl_handle) curl_easy_cleanup(context->curl_handle); 
    free(context->api_key);
    free(context->api_base_url);
    free(context);
}

static char* build_openai_json_payload(const dpt_request_config_t* request_config) {
    char* payload = NULL;
    char* messages_json_array = NULL;
    size_t current_messages_len = 0;
    
    messages_json_array = duplicate_string("[");
    if (!messages_json_array) return NULL;
    current_messages_len = 1;

    for (size_t i = 0; i < request_config->num_messages; ++i) {
        const dpt_message_t* msg = &request_config->messages[i]; 
        char* role_str = NULL;
        switch (msg->role) { 
            case DPT_ROLE_SYSTEM: role_str = "system"; break;
            case DPT_ROLE_USER: role_str = "user"; break;
            case DPT_ROLE_ASSISTANT: role_str = "assistant"; break;
            case DPT_ROLE_TOOL: role_str = "tool"; break;
            default: role_str = "user"; 
        }

        char* content_json = NULL;
        if (msg->num_parts == 1 && msg->parts[0].type == DPT_CONTENT_PART_TEXT) { 
            char* escaped_text = escape_json_string(msg->parts[0].text);
            if (!escaped_text) { free(messages_json_array); return NULL; }
            if (asprintf(&content_json, "\"content\": %s", escaped_text) < 0) {
                free(escaped_text); free(messages_json_array); return NULL;
            }
            free(escaped_text);
        } else { 
            char* content_array_str = duplicate_string("[");
            size_t current_content_array_len = 1;
            if (!content_array_str) { free(messages_json_array); return NULL; }

            for (size_t j = 0; j < msg->num_parts; ++j) {
                char* part_json = NULL;
                if (msg->parts[j].type == DPT_CONTENT_PART_TEXT) { 
                    char* escaped_text = escape_json_string(msg->parts[j].text);
                    if(!escaped_text) { free(content_array_str); free(messages_json_array); return NULL; }
                    if (asprintf(&part_json, "{\"type\": \"text\", \"text\": %s}", escaped_text) < 0) {
                        free(escaped_text); free(content_array_str); free(messages_json_array); return NULL;
                    }
                    free(escaped_text);
                } else if (msg->parts[j].type == DPT_CONTENT_PART_IMAGE_URL) { 
                    char* escaped_url = escape_json_string(msg->parts[j].image_url);
                     if(!escaped_url) { free(content_array_str); free(messages_json_array); return NULL; }
                    if (asprintf(&part_json, "{\"type\": \"image_url\", \"image_url\": {\"url\": %s}}", escaped_url) < 0) {
                        free(escaped_url); free(content_array_str); free(messages_json_array); return NULL;
                    }
                    free(escaped_url);
                }
                if (part_json) {
                    size_t part_json_len = strlen(part_json);
                    char* temp_content_array = realloc(content_array_str, current_content_array_len + part_json_len + (j > 0 ? 1 : 0) + 1);
                    if (!temp_content_array) { free(part_json); free(content_array_str); free(messages_json_array); return NULL; }
                    content_array_str = temp_content_array;
                    if (j > 0) strcat(content_array_str, ",");
                    strcat(content_array_str, part_json);
                    current_content_array_len += part_json_len + (j > 0 ? 1 : 0);
                    free(part_json);
                }
            }
            char* temp_content_array = realloc(content_array_str, current_content_array_len + 1 + 1); 
            if (!temp_content_array) { free(content_array_str); free(messages_json_array); return NULL; }
            content_array_str = temp_content_array;
            strcat(content_array_str, "]");
            current_content_array_len++;
            
            if (asprintf(&content_json, "\"content\": %s", content_array_str) < 0) {
                free(content_array_str); free(messages_json_array); return NULL;
            }
            free(content_array_str);
        }
        
        char* message_obj_str;
        if (asprintf(&message_obj_str, "{\"role\": \"%s\", %s}", role_str, content_json) < 0) {
            free(content_json); free(messages_json_array); return NULL;
        }
        free(content_json);

        size_t message_obj_len = strlen(message_obj_str);
        char* temp_messages_array = realloc(messages_json_array, current_messages_len + message_obj_len + (i > 0 ? 1 : 0) + 1);
        if (!temp_messages_array) { free(message_obj_str); free(messages_json_array); return NULL; }
        messages_json_array = temp_messages_array;

        if (i > 0) strcat(messages_json_array, ",");
        strcat(messages_json_array, message_obj_str);
        current_messages_len += message_obj_len + (i > 0 ? 1 : 0);
        free(message_obj_str);
    }
    char* temp_messages_array = realloc(messages_json_array, current_messages_len + 1 + 1); 
    if (!temp_messages_array) { free(messages_json_array); return NULL; }
    messages_json_array = temp_messages_array;
    strcat(messages_json_array, "]");

    char stream_json_part[20]; 
    if (request_config->stream) {
        strcpy(stream_json_part, ", \"stream\": true");
    } else {
        stream_json_part[0] = '\0';
    }
    
    if (asprintf(&payload, "{\"model\": \"%s\", \"messages\": %s, \"temperature\": %.2f, \"max_tokens\": %d%s}",
             request_config->model, messages_json_array, request_config->temperature,
             request_config->max_tokens > 0 ? request_config->max_tokens : 1024, 
             stream_json_part) < 0) {
        payload = NULL;
    }
    free(messages_json_array);
    return payload;
}

static char* build_gemini_json_payload(const dpt_request_config_t* request_config) {
    char* payload = NULL;
    char* contents_json_array = NULL;
    size_t current_contents_len = 0;

    contents_json_array = duplicate_string("[");
    if (!contents_json_array) return NULL;
    current_contents_len = 1;

    for (size_t i = 0; i < request_config->num_messages; ++i) {
        const dpt_message_t* msg = &request_config->messages[i]; 
        char* role_str = (msg->role == DPT_ROLE_ASSISTANT) ? "model" : "user"; 

        char* parts_json_array = duplicate_string("[");
        size_t current_parts_len = 1;
        if (!parts_json_array) { free(contents_json_array); return NULL; }

        for (size_t j = 0; j < msg->num_parts; ++j) {
            const dpt_content_part_t* part = &msg->parts[j]; 
            char* part_json = NULL;
            if (part->type == DPT_CONTENT_PART_TEXT) { 
                char* escaped_text = escape_json_string(part->text);
                if (!escaped_text) { free(parts_json_array); free(contents_json_array); return NULL; }
                if (asprintf(&part_json, "{\"text\": %s}", escaped_text) < 0) {
                     free(escaped_text); free(parts_json_array); free(contents_json_array); return NULL;
                }
                free(escaped_text);
            } else if (part->type == DPT_CONTENT_PART_IMAGE_BASE64) { 
                char* escaped_mimetype = escape_json_string(part->image_base64.mime_type);
                char* escaped_data = escape_json_string(part->image_base64.data); 
                if (!escaped_mimetype || !escaped_data) {
                    free(escaped_mimetype); free(escaped_data);
                    free(parts_json_array); free(contents_json_array); return NULL;
                }
                if (asprintf(&part_json, "{\"inline_data\": {\"mime_type\": %s, \"data\": %s}}",
                         escaped_mimetype, escaped_data) < 0) {
                    free(escaped_mimetype); free(escaped_data);
                    free(parts_json_array); free(contents_json_array); return NULL;
                }
                free(escaped_mimetype); free(escaped_data);
            } else if (part->type == DPT_CONTENT_PART_IMAGE_URL) { 
                char* url_as_text;
                if (asprintf(&url_as_text, "Image at URL: %s (Note: direct URL processing may vary by Gemini model)", part->image_url) < 0) {
                    free(parts_json_array); free(contents_json_array); return NULL;
                }
                char* escaped_text = escape_json_string(url_as_text);
                free(url_as_text);
                if (!escaped_text) { free(parts_json_array); free(contents_json_array); return NULL; }
                if (asprintf(&part_json, "{\"text\": %s}", escaped_text) < 0) {
                     free(escaped_text); free(parts_json_array); free(contents_json_array); return NULL;
                }
                free(escaped_text);
            }

            if (part_json) {
                size_t part_json_len = strlen(part_json);
                char* temp_parts_array = realloc(parts_json_array, current_parts_len + part_json_len + (j > 0 ? 1 : 0) + 1);
                if (!temp_parts_array) { free(part_json); free(parts_json_array); free(contents_json_array); return NULL; }
                parts_json_array = temp_parts_array;
                if (j > 0) strcat(parts_json_array, ",");
                strcat(parts_json_array, part_json);
                current_parts_len += part_json_len + (j > 0 ? 1 : 0);
                free(part_json);
            }
        }
        char* temp_parts_array = realloc(parts_json_array, current_parts_len + 1 + 1);
        if(!temp_parts_array) { free(parts_json_array); free(contents_json_array); return NULL; }
        parts_json_array = temp_parts_array;
        strcat(parts_json_array, "]");
        current_parts_len++;
        
        char* content_obj_str;
        if (asprintf(&content_obj_str, "{\"parts\": %s, \"role\": \"%s\"}", parts_json_array, role_str) < 0) {
            free(parts_json_array); free(contents_json_array); return NULL;
        }
        free(parts_json_array);

        size_t content_obj_len = strlen(content_obj_str);
        char* temp_contents_array = realloc(contents_json_array, current_contents_len + content_obj_len + (i > 0 ? 1 : 0) + 1);
        if(!temp_contents_array) { free(content_obj_str); free(contents_json_array); return NULL; }
        contents_json_array = temp_contents_array;

        if (i > 0) strcat(contents_json_array, ",");
        strcat(contents_json_array, content_obj_str);
        current_contents_len += content_obj_len + (i > 0 ? 1 : 0);
        free(content_obj_str);
    }
    char* temp_contents_array = realloc(contents_json_array, current_contents_len + 1 + 1);
    if(!temp_contents_array) { free(contents_json_array); return NULL; }
    contents_json_array = temp_contents_array;
    strcat(contents_json_array, "]");

    char generation_config_json[256];
    snprintf(generation_config_json, sizeof(generation_config_json),
             "\"generationConfig\": {\"temperature\": %.2f, \"maxOutputTokens\": %d}",
             request_config->temperature, 
             request_config->max_tokens > 0 ? request_config->max_tokens : 1024); 

    if (asprintf(&payload, "{\"contents\": %s, %s}", contents_json_array, generation_config_json) < 0) {
        payload = NULL; 
    }
    free(contents_json_array);
    return payload;
}

static char* extract_text_from_full_response_json(const char* json_response, dpt_provider_type_t provider) {
    if (!json_response) return NULL;
    char* text = NULL;
    if (provider == DPT_PROVIDER_OPENAI_COMPATIBLE) {
        const char* choices_key = "\"choices\": [";
        char* choices_start = strstr(json_response, choices_key);
        if (choices_start) {
            const char* message_key = "\"message\": {";
            char* message_start = strstr(choices_start, message_key);
            if (message_start) {
                const char* content_key = "\"content\": \"";
                char* content_start = strstr(message_start, content_key);
                if (content_start) {
                    content_start += strlen(content_key);
                    char* content_end = strchr(content_start, '"');
                    if (content_end) {
                        size_t len = content_end - content_start;
                        text = malloc(len + 1);
                        if (text) {
                            strncpy(text, content_start, len);
                            text[len] = '\0';
                            char *r = text, *w = text; while (*r) { if (*r == '\\' && *(r+1)) { r++; switch(*r) { case 'n': *w++ = '\n'; break; case '"': *w++ = '"'; break; case '\\': *w++ = '\\'; break; case 't': *w++ = '\t'; break; default: *w++ = '\\'; *w++ = *r; break; } } else { *w++ = *r; } r++; } *w = '\0';
                        }
                    }
                }
            }
        }
    } else if (provider == DPT_PROVIDER_GOOGLE_GEMINI) {
        const char* candidates_key = "\"candidates\": [";
        char* candidates_start = strstr(json_response, candidates_key);
        if (candidates_start) {
            const char* content_key_gemini = "\"content\": {"; 
            char* content_start_gemini = strstr(candidates_start, content_key_gemini);
            if (content_start_gemini) {
                const char* parts_key = "\"parts\": [";
                char* parts_start = strstr(content_start_gemini, parts_key);
                if (parts_start) {
                    const char* text_key = "\"text\": \"";
                    char* text_start = strstr(parts_start, text_key);
                    if (text_start) {
                        text_start += strlen(text_key);
                        char* text_end = strchr(text_start, '"');
                        if (text_end) {
                            size_t len = text_end - text_start;
                            text = malloc(len + 1);
                            if (text) {
                                strncpy(text, text_start, len);
                                text[len] = '\0';
                                char *r = text, *w = text; while (*r) { if (*r == '\\' && *(r+1)) { r++; switch(*r) { case 'n': *w++ = '\n'; break; case '"': *w++ = '"'; break; case '\\': *w++ = '\\'; break; case 't': *w++ = '\t'; break; default: *w++ = '\\'; *w++ = *r; break; } } else { *w++ = *r; } r++; } *w = '\0';
                            }
                        }
                    }
                }
            }
        }
    }
    return text;
}

static char* extract_openai_finish_reason(const char* json_chunk) {
    const char* reason_key = "\"finish_reason\": \"";
    char* reason_start = strstr(json_chunk, reason_key);
    if (reason_start) {
        reason_start += strlen(reason_key);
        char* reason_end = strchr(reason_start, '"');
        if (reason_end) {
            size_t len = reason_end - reason_start;
            if (len == 4 && strncmp(reason_start, "null", 4) == 0) {
                return NULL; 
            }
            char* reason = malloc(len + 1);
            if (reason) {
                strncpy(reason, reason_start, len);
                reason[len] = '\0';
                return reason;
            }
        }
    }
    return NULL; 
}

static size_t streaming_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    stream_processor_t* processor = (stream_processor_t*)userp;

    if (processor->stop_streaming_signal) {
        return 0; 
    }

    size_t needed_capacity = processor->buffer_size + realsize + 1;
    if (processor->buffer_capacity < needed_capacity) {
        size_t new_capacity = needed_capacity > processor->buffer_capacity * 2 ? needed_capacity : processor->buffer_capacity * 2;
        if (new_capacity < 256) new_capacity = 256; 
        char* new_buf = realloc(processor->buffer, new_capacity);
        if (!new_buf) {
            const char* err_msg = "Memory allocation failed in stream buffer";
            processor->user_callback(NULL, processor->user_data, true, err_msg);
            if (!processor->accumulated_error_during_stream) processor->accumulated_error_during_stream = duplicate_string(err_msg);
            return 0; 
        }
        processor->buffer = new_buf;
        processor->buffer_capacity = new_capacity;
    }
    memcpy(processor->buffer + processor->buffer_size, contents, realsize);
    processor->buffer_size += realsize;
    processor->buffer[processor->buffer_size] = '\0';

    char* current_event_start = processor->buffer;
    size_t remaining_buffer_size = processor->buffer_size;

    while (true) {
        if (processor->stop_streaming_signal) break;
        char* extracted_token = NULL;
        bool is_final_callback_for_chunk = false;
        char* parse_error = NULL;

        // Unified SSE parsing for OpenAI and Gemini (when alt=sse)
        if (processor->provider == DPT_PROVIDER_OPENAI_COMPATIBLE || 
            (processor->provider == DPT_PROVIDER_GOOGLE_GEMINI)) { 
            
            char* event_end = strstr(current_event_start, "\n\n");
            if (!event_end) break; 

            size_t event_len = event_end - current_event_start;
            // Using malloc for temp_event_data to avoid VLA issues with some compilers/standards
            char* temp_event_data = malloc(event_len + 1);
            if (!temp_event_data) { parse_error = "Memory allocation for event data failed"; is_final_callback_for_chunk = true; break; }
            strncpy(temp_event_data, current_event_start, event_len);
            temp_event_data[event_len] = '\0';

            current_event_start = event_end + 2; 
            remaining_buffer_size -= (event_len + 2);

            char* line = temp_event_data;
            while (line && *line) {
                char* next_line = strchr(line, '\n');
                if (next_line) *next_line = '\0'; 

                if (strncmp(line, "data: ", 6) == 0) {
                    char* json_data = line + 6;
                    if (strcmp(json_data, "[DONE]") == 0 && processor->provider == DPT_PROVIDER_OPENAI_COMPATIBLE) {
                        is_final_callback_for_chunk = true;
                        if (!processor->finish_reason_capture) processor->finish_reason_capture = duplicate_string("done_marker");
                        break; 
                    }
                    
                    const char* delta_key = "\"delta\":"; 
                    const char* content_key_openai = "\"content\": \"";
                    const char* text_key_gemini = "\"text\": \""; 
                    
                    char* token_start = NULL;
                    char* token_end = NULL;

                    if (processor->provider == DPT_PROVIDER_OPENAI_COMPATIBLE) {
                        char* delta_start = strstr(json_data, delta_key);
                        if (delta_start) {
                            token_start = strstr(delta_start, content_key_openai);
                            if (token_start) {
                                token_start += strlen(content_key_openai);
                                token_end = strchr(token_start, '"');
                            }
                        }
                         if (!processor->finish_reason_capture) {
                            char* fr = extract_openai_finish_reason(json_data);
                            if (fr) {
                                processor->finish_reason_capture = fr;
                                is_final_callback_for_chunk = true; 
                            }
                        }
                    } else if (processor->provider == DPT_PROVIDER_GOOGLE_GEMINI) {
                        const char* candidates_key = "\"candidates\":";
                        char* candidates_start = strstr(json_data, candidates_key);
                        if(candidates_start) {
                            token_start = strstr(candidates_start, text_key_gemini);
                            if (token_start) {
                                token_start += strlen(text_key_gemini);
                                token_end = strchr(token_start, '"');
                            }
                        }
                        const char* finish_reason_key_gemini = "\"finishReason\": \"";
                        char* fr_start_gemini = strstr(json_data, finish_reason_key_gemini); 
                        if (fr_start_gemini && !processor->finish_reason_capture) {
                            fr_start_gemini += strlen(finish_reason_key_gemini);
                            char* fr_end_gemini = strchr(fr_start_gemini, '"');
                            if (fr_end_gemini) {
                                size_t fr_len = fr_end_gemini - fr_start_gemini;
                                processor->finish_reason_capture = malloc(fr_len + 1);
                                if (processor->finish_reason_capture) {
                                    strncpy(processor->finish_reason_capture, fr_start_gemini, fr_len);
                                    (processor->finish_reason_capture)[fr_len] = '\0';
                                    is_final_callback_for_chunk = true;
                                }
                            }
                        }
                    }

                    if (token_start && token_end) {
                        size_t token_len = token_end - token_start;
                        if (token_len > 0) { 
                            extracted_token = malloc(token_len + 1);
                            if (extracted_token) {
                                strncpy(extracted_token, token_start, token_len);
                                extracted_token[token_len] = '\0';
                                char *r = extracted_token, *w = extracted_token; while (*r) { if (*r == '\\' && *(r+1)) { r++; switch(*r) { case 'n': *w++ = '\n'; break; case '"': *w++ = '"'; break; case '\\': *w++ = '\\'; break; case 't': *w++ = '\t'; break; default: *w++ = '\\'; *w++ = *r; break; } } else { *w++ = *r; } r++; } *w = '\0';
                            } else { parse_error = "Memory allocation failed for token"; }
                        }
                    }
                }
                if (next_line) line = next_line + 1; else break;
            }
            free(temp_event_data); // Free malloc'd event data
        }

        if (parse_error) {
            if (processor->user_callback(NULL, processor->user_data, true, parse_error) != 0) {
                processor->stop_streaming_signal = true;
            }
            if (!processor->accumulated_error_during_stream) processor->accumulated_error_during_stream = duplicate_string(parse_error);
             // parse_error is a literal, no free
        } else if (extracted_token) {
            if (processor->user_callback(extracted_token, processor->user_data, is_final_callback_for_chunk, NULL) != 0) {
                processor->stop_streaming_signal = true;
            }
            free(extracted_token);
        } else if (is_final_callback_for_chunk) { 
            if (processor->user_callback(NULL, processor->user_data, true, NULL) != 0) {
                processor->stop_streaming_signal = true;
            }
        }
        if (is_final_callback_for_chunk) processor->stop_streaming_signal = true; 
    } 

    if (remaining_buffer_size > 0 && current_event_start > processor->buffer) {
        memmove(processor->buffer, current_event_start, remaining_buffer_size);
    }
    processor->buffer_size = remaining_buffer_size;
    if (processor->buffer_size < processor->buffer_capacity) { // Ensure null termination if space allows
        processor->buffer[processor->buffer_size] = '\0'; 
    }


    return realsize; 
}


int dpt_perform_completion(dpt_context_t* context,
                          const dpt_request_config_t* request_config,
                          dpt_response_t* response) {
    if (!context || !request_config || !response) {
        if (response) response->error_message = duplicate_string("Invalid arguments to dpt_perform_completion.");
        return -1;
    }
    if (request_config->stream) {
        if (response) response->error_message = duplicate_string("dpt_perform_completion called with stream=true. Use dpt_perform_streaming_completion instead.");
        return -1;
    }

    memset(response, 0, sizeof(dpt_response_t)); 

    CURL* curl = curl_easy_init(); 
    if (!curl) {
        response->error_message = duplicate_string("Failed to initialize curl handle for non-streaming.");
        return -1;
    }

    CURLcode res;
    struct curl_slist* headers = NULL;
    memory_struct_t chunk_mem;
    chunk_mem.memory = malloc(1); 
    chunk_mem.size = 0;
    if (!chunk_mem.memory) {
        response->error_message = duplicate_string("Failed to allocate memory for response buffer.");
        curl_easy_cleanup(curl); 
        return -1;
    }

    char* json_payload = NULL;
    char url[1024];

    if (context->provider == DPT_PROVIDER_OPENAI_COMPATIBLE) {
        json_payload = build_openai_json_payload(request_config); 
        snprintf(url, sizeof(url), "%s/chat/completions", context->api_base_url);
        headers = curl_slist_append(headers, "Content-Type: application/json");
        char auth_header[256];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", context->api_key);
        headers = curl_slist_append(headers, auth_header);
    } else if (context->provider == DPT_PROVIDER_GOOGLE_GEMINI) {
        json_payload = build_gemini_json_payload(request_config);
        snprintf(url, sizeof(url), "%s/models/%s:generateContent?key=%s",
                 context->api_base_url, request_config->model, context->api_key);
        headers = curl_slist_append(headers, "Content-Type: application/json");
    } else {
        response->error_message = duplicate_string("Unsupported LLM provider.");
        free(chunk_mem.memory); curl_easy_cleanup(curl); 
        return -1;
    }

    if (!json_payload) {
        response->error_message = duplicate_string("Failed to build JSON payload.");
        free(chunk_mem.memory); curl_slist_free_all(headers); curl_easy_cleanup(curl); 
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk_mem);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, DIASTERPARTY_USER_AGENT); 
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_status_code);

    if (res != CURLE_OK) {
        asprintf(&response->error_message, "curl_easy_perform() failed: %s (HTTP status: %ld)",
                 curl_easy_strerror(res), response->http_status_code);
    } else {
        if (response->http_status_code >= 200 && response->http_status_code < 300) {
            char* extracted_text = extract_text_from_full_response_json(chunk_mem.memory, context->provider);
            if (extracted_text) {
                response->parts = calloc(1, sizeof(dpt_response_part_t)); 
                if (response->parts) {
                    response->num_parts = 1;
                    response->parts[0].type = DPT_CONTENT_PART_TEXT; 
                    response->parts[0].text = extracted_text; 
                } else {
                    free(extracted_text);
                    response->error_message = duplicate_string("Failed to allocate memory for response part.");
                }
                if (context->provider == DPT_PROVIDER_OPENAI_COMPATIBLE) {
                    const char* choices_key = "\"choices\": [";
                    char* choices_start = strstr(chunk_mem.memory, choices_key);
                    if(choices_start) { // Search within the choices array for finish_reason
                        response->finish_reason = extract_openai_finish_reason(choices_start);
                    }
                }
                if (context->provider == DPT_PROVIDER_GOOGLE_GEMINI) {
                    const char* feedback_key = "\"promptFeedback\":"; // Gemini specific
                    char* feedback_start = strstr(chunk_mem.memory, feedback_key);
                    if (feedback_start) {
                        const char* reason_key = "\"finishReason\": \"";
                        char* reason_start = strstr(feedback_start, reason_key);
                        if (reason_start) {
                            reason_start += strlen(reason_key);
                            char* reason_end = strchr(reason_start, '"');
                            if (reason_end) {
                                size_t len = reason_end - reason_start;
                                response->finish_reason = malloc(len + 1);
                                if (response->finish_reason) {
                                    strncpy(response->finish_reason, reason_start, len);
                                    (response->finish_reason)[len] = '\0';
                                }
                            }
                        }
                    }
                }
            } else { 
                const char* error_key_marker = "\"error\""; 
                char* error_json_start = strstr(chunk_mem.memory, error_key_marker);
                if (error_json_start) {
                    const char* message_key = "\"message\": \"";
                    char* message_start = strstr(error_json_start, message_key);
                    if (message_start) {
                        message_start += strlen(message_key);
                        char* message_end = strchr(message_start, '"');
                        if (message_end) {
                            size_t len = message_end - message_start;
                            char* api_err_msg = malloc(len + 1);
                            if (api_err_msg) {
                                strncpy(api_err_msg, message_start, len); api_err_msg[len] = '\0';
                                asprintf(&response->error_message, "API error (HTTP %ld): %s", response->http_status_code, api_err_msg);
                                free(api_err_msg);
                            }
                        }
                    }
                }
                if (!response->error_message) { 
                    asprintf(&response->error_message, "Failed to parse successful response (HTTP %ld). Body: %.200s...", response->http_status_code, chunk_mem.memory ? chunk_mem.memory : "(empty)");
                }
            }
        } else { 
            asprintf(&response->error_message, "HTTP error %ld: %.500s", response->http_status_code, chunk_mem.memory ? chunk_mem.memory : "(no response body)");
        }
    }

    free(json_payload);
    free(chunk_mem.memory);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    return response->error_message ? -1 : 0;
}


int dpt_perform_streaming_completion(dpt_context_t* context,
                                    const dpt_request_config_t* request_config,
                                    dpt_stream_callback_t callback,
                                    void* user_data,
                                    dpt_response_t* response) {
    if (!context || !request_config || !callback || !response) {
        if (response) response->error_message = duplicate_string("Invalid arguments to dpt_perform_streaming_completion.");
        return -1;
    }
    if (context->provider == DPT_PROVIDER_OPENAI_COMPATIBLE && !request_config->stream) {
         if (response) response->error_message = duplicate_string("dpt_perform_streaming_completion for OpenAI requires stream=true in config.");
        return -1;
    }

    memset(response, 0, sizeof(dpt_response_t));

    CURL* curl = curl_easy_init(); 
    if (!curl) {
        response->error_message = duplicate_string("Failed to initialize curl handle for streaming.");
        return -1;
    }

    stream_processor_t processor = {0};
    processor.user_callback = callback;
    processor.user_data = user_data;
    processor.provider = context->provider;
    processor.buffer_capacity = 4096; // Increased initial buffer for streams
    processor.buffer = malloc(processor.buffer_capacity);
    if (!processor.buffer) {
        response->error_message = duplicate_string("Failed to allocate stream processor buffer.");
        curl_easy_cleanup(curl); 
        return -1;
    }
    processor.buffer[0] = '\0';

    char* json_payload = NULL;
    char url[1024];
    struct curl_slist* headers = NULL;

    if (context->provider == DPT_PROVIDER_OPENAI_COMPATIBLE) {
        json_payload = build_openai_json_payload(request_config);
        snprintf(url, sizeof(url), "%s/chat/completions", context->api_base_url);
        headers = curl_slist_append(headers, "Content-Type: application/json");
        char auth_header[256];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", context->api_key);
        headers = curl_slist_append(headers, auth_header);
    } else if (context->provider == DPT_PROVIDER_GOOGLE_GEMINI) {
        json_payload = build_gemini_json_payload(request_config); 
        snprintf(url, sizeof(url), "%s/models/%s:streamGenerateContent?key=%s&alt=sse", 
                 context->api_base_url, request_config->model, context->api_key);
        headers = curl_slist_append(headers, "Content-Type: application/json");
    } else {
        response->error_message = duplicate_string("Unsupported LLM provider for streaming.");
        free(processor.buffer); curl_easy_cleanup(curl); 
        return -1;
    }
    
    if (!json_payload) {
        response->error_message = duplicate_string("Failed to build JSON payload for streaming.");
        free(processor.buffer); curl_slist_free_all(headers); curl_easy_cleanup(curl); 
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, streaming_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&processor);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, DIASTERPARTY_USER_AGENT); 
    // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_status_code);

    if (!processor.stop_streaming_signal && res == CURLE_OK && response->http_status_code >= 200 && response->http_status_code < 300) {
        // If stream ended cleanly from server side but callback hasn't been told it's final (e.g. no [DONE] or explicit finish_reason)
        // Call it one last time.
        processor.user_callback(NULL, processor.user_data, true, processor.accumulated_error_during_stream);
    } else if (res != CURLE_OK) {
        char curl_err_buf[CURL_ERROR_SIZE];
        snprintf(curl_err_buf, CURL_ERROR_SIZE, "curl_easy_perform() failed: %s", curl_easy_strerror(res));
        if (!processor.stop_streaming_signal) { 
             processor.user_callback(NULL, processor.user_data, true, curl_err_buf);
        }
        if (!response->error_message) response->error_message = duplicate_string(curl_err_buf);
    } else if (response->http_status_code < 200 || response->http_status_code >= 300) {
        char http_err_buf[1024]; 
        snprintf(http_err_buf, sizeof(http_err_buf), "HTTP error %ld. Response body hint: %.500s", 
                response->http_status_code, 
                processor.buffer_size > 0 ? processor.buffer : "(empty/processed)");
        if (!processor.stop_streaming_signal) {
            processor.user_callback(NULL, processor.user_data, true, http_err_buf);
        }
        if (!response->error_message) response->error_message = duplicate_string(http_err_buf);
    }

    if (processor.finish_reason_capture) {
        response->finish_reason = processor.finish_reason_capture; 
    } else if (res == CURLE_OK && response->http_status_code >=200 && response->http_status_code < 300 && !processor.accumulated_error_during_stream && !response->finish_reason) {
        // If stream finished cleanly (HTTP OK, no stream errors) but no specific reason was parsed, mark as completed.
        response->finish_reason = duplicate_string("completed");
    }


    if (processor.accumulated_error_during_stream) {
        if (response->error_message) { 
            char* temp_err;
            // Concatenate, ensuring not to duplicate if it's the same error reported at HTTP level
            if (strstr(response->error_message, processor.accumulated_error_during_stream) == NULL) {
                 asprintf(&temp_err, "%s; Stream processing error: %s", response->error_message, processor.accumulated_error_during_stream);
                 free(response->error_message);
                 response->error_message = temp_err;
            }
            free(processor.accumulated_error_during_stream); 
        } else {
            response->error_message = processor.accumulated_error_during_stream; 
        }
    }

    free(json_payload);
    free(processor.buffer);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    return response->error_message ? -1 : 0;
}


void dpt_free_response_content(dpt_response_t* response) {
    if (!response) return;
    if (response->parts) {
        for (size_t i = 0; i < response->num_parts; ++i) {
            free(response->parts[i].text);
        }
        free(response->parts);
        response->parts = NULL;
        response->num_parts = 0;
    }
    free(response->error_message);
    response->error_message = NULL;
    free(response->finish_reason);
    response->finish_reason = NULL;
}

void dpt_free_messages(dpt_message_t* messages, size_t num_messages) {
    if (!messages) return;
    for (size_t i = 0; i < num_messages; ++i) {
        if (messages[i].parts) {
            for (size_t j = 0; j < messages[i].num_parts; ++j) {
                dpt_content_part_t* part = &messages[i].parts[j]; 
                free(part->text); 
                free(part->image_url);
                if (part->type == DPT_CONTENT_PART_IMAGE_BASE64) { 
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

static bool dpt_message_add_part_internal(dpt_message_t* message, dpt_content_part_type_t type, 
                                   const char* text_content, 
                                   const char* image_url_content, 
                                   const char* mime_type_content, const char* base64_data_content) {
    if (!message) return false;

    dpt_content_part_t* new_parts_array = realloc(message->parts, (message->num_parts + 1) * sizeof(dpt_content_part_t));
    if (!new_parts_array) return false;
    message->parts = new_parts_array;

    dpt_content_part_t* new_part = &message->parts[message->num_parts];
    memset(new_part, 0, sizeof(dpt_content_part_t)); 
    new_part->type = type;

    bool success = false;
    if (type == DPT_CONTENT_PART_TEXT && text_content) {
        new_part->text = duplicate_string(text_content);
        success = (new_part->text != NULL);
    } else if (type == DPT_CONTENT_PART_IMAGE_URL && image_url_content) {
        new_part->image_url = duplicate_string(image_url_content);
        success = (new_part->image_url != NULL);
    } else if (type == DPT_CONTENT_PART_IMAGE_BASE64 && mime_type_content && base64_data_content) {
        new_part->image_base64.mime_type = duplicate_string(mime_type_content);
        new_part->image_base64.data = duplicate_string(base64_data_content); 
        success = (new_part->image_base64.mime_type != NULL && new_part->image_base64.data != NULL);
        if (!success) {
            free(new_part->image_base64.mime_type); new_part->image_base64.mime_type = NULL;
            free(new_part->image_base64.data); new_part->image_base64.data = NULL;
        }
    } else {
        return false; 
    }

    if (success) {
        message->num_parts++;
    } else {
        fprintf(stderr, "Failed to allocate memory for content part.\n");
    }
    return success;
}


bool dpt_message_add_text_part(dpt_message_t* message, const char* text) {
    return dpt_message_add_part_internal(message, DPT_CONTENT_PART_TEXT, text, NULL, NULL, NULL);
}

bool dpt_message_add_image_url_part(dpt_message_t* message, const char* image_url) {
    return dpt_message_add_part_internal(message, DPT_CONTENT_PART_IMAGE_URL, NULL, image_url, NULL, NULL);
}

bool dpt_message_add_base64_image_part(dpt_message_t* message, const char* mime_type, const char* base64_data) {
    return dpt_message_add_part_internal(message, DPT_CONTENT_PART_IMAGE_BASE64, NULL, NULL, mime_type, base64_data);
}

