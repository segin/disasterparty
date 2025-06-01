#define _GNU_SOURCE 
#include "disasterparty.h" 
#include <curl/curl.h>
#include <cjson/cJSON.h> 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h> // For isprint

// Default base URLs
const char* DEFAULT_OPENAI_API_BASE_URL = "https://api.openai.com/v1";
const char* DEFAULT_GEMINI_API_BASE_URL = "https://generativelanguage.googleapis.com/v1beta";
const char* DISASTERPARTY_USER_AGENT = "disasterparty_c/" DP_VERSION;


// Internal context structure
struct dp_context_s {
    dp_provider_type_t provider;
    char* api_key;
    char* api_base_url;
};

typedef struct {
    char* memory;
    size_t size;
} memory_struct_t;

typedef struct {
    dp_stream_callback_t user_callback;
    void* user_data;
    char* buffer;
    size_t buffer_size;
    size_t buffer_capacity;
    dp_provider_type_t provider;
    char* finish_reason_capture;
    bool stop_streaming_signal;
    char* accumulated_error_during_stream;
} stream_processor_t;

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
        context->api_base_url = dp_internal_strdup(
            provider == DP_PROVIDER_OPENAI_COMPATIBLE ? DEFAULT_OPENAI_API_BASE_URL : DEFAULT_GEMINI_API_BASE_URL);
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
                        fprintf(stderr, "Error: malloc failed for data_uri in OpenAI payload.\n");
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

static char* extract_text_from_full_response_with_cjson(const char* json_response_str, dp_provider_type_t provider, char** finish_reason_out) {
    if (finish_reason_out) *finish_reason_out = NULL;
    if (!json_response_str) return NULL;

    cJSON *root = cJSON_Parse(json_response_str);
    if (!root) {
        return NULL;
    }

    char* extracted_text = NULL;
    cJSON *choices_array = NULL;
    cJSON *candidates_array = NULL;

    if (provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
        choices_array = cJSON_GetObjectItemCaseSensitive(root, "choices");
        if (cJSON_IsArray(choices_array) && cJSON_GetArraySize(choices_array) > 0) {
            cJSON *first_choice = cJSON_GetArrayItem(choices_array, 0);
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
        candidates_array = cJSON_GetObjectItemCaseSensitive(root, "candidates");
        if (cJSON_IsArray(candidates_array) && cJSON_GetArraySize(candidates_array) > 0) {
            cJSON *first_candidate = cJSON_GetArrayItem(candidates_array, 0);
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
    }
    
    cJSON_Delete(root);
    return extracted_text;
}

static size_t streaming_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    stream_processor_t* processor = (stream_processor_t*)userp;

    if (processor->provider == DP_PROVIDER_GOOGLE_GEMINI) {
        fprintf(stderr, "[DEBUG-GEMINI-LIBCURL-CHUNK] Received %zu bytes from libcurl. Current buffer size: %zu\n", realsize, processor->buffer_size);
        // fwrite(contents, 1, realsize, stderr); // Print raw bytes
        // fprintf(stderr, "\n[DEBUG-GEMINI-LIBCURL-CHUNK-END]\n");
        // fflush(stderr);
    }

    if (processor->stop_streaming_signal) return 0; // User signaled to stop

    // Append new data to processor's buffer
    size_t needed_capacity = processor->buffer_size + realsize + 1; // +1 for null terminator
    if (processor->buffer_capacity < needed_capacity) {
        size_t new_capacity = needed_capacity > processor->buffer_capacity * 2 ? needed_capacity : processor->buffer_capacity * 2;
        if (new_capacity < 1024) new_capacity = 1024; // Ensure a minimum reasonable capacity
        char* new_buf = realloc(processor->buffer, new_capacity);
        if (!new_buf) {
            const char* err_msg = "Stream buffer memory re-allocation failed";
            processor->user_callback(NULL, processor->user_data, true, err_msg);
            if (!processor->accumulated_error_during_stream) processor->accumulated_error_during_stream = dp_internal_strdup(err_msg);
            return 0; // Signal error to libcurl
        }
        processor->buffer = new_buf;
        processor->buffer_capacity = new_capacity;
    }
    memcpy(processor->buffer + processor->buffer_size, contents, realsize);
    processor->buffer_size += realsize;
    processor->buffer[processor->buffer_size] = '\0'; // Null-terminate the buffer

    if (processor->provider == DP_PROVIDER_GOOGLE_GEMINI) {
        fprintf(stderr, "[DEBUG-GEMINI-BUFFER-CONTENT-START] Accumulated buffer (size %zu):\n<", processor->buffer_size);
        for(size_t k=0; k < processor->buffer_size; ++k) {
            char c = processor->buffer[k];
            if (c == '\n') fprintf(stderr, "\\n");
            else if (c == '\r') fprintf(stderr, "\\r");
            else if (isprint(c)) fprintf(stderr, "%c", c);
            else fprintf(stderr, "[%02X]", (unsigned char)c);
        }
        fprintf(stderr, ">\n[DEBUG-GEMINI-BUFFER-CONTENT-END]\n");
        fflush(stderr);
    }


    char* current_event_start = processor->buffer;
    size_t remaining_in_buffer = processor->buffer_size;

    while (true) {
        if (processor->stop_streaming_signal) break;

        if (processor->provider == DP_PROVIDER_GOOGLE_GEMINI) {
            fprintf(stderr, "[DEBUG-GEMINI-SSE-LOOP-TOP] current_event_start points to: <%.30s> remaining_in_buffer: %zu\n", current_event_start, remaining_in_buffer);
            fflush(stderr);
        }
        
        char* event_end = strstr(current_event_start, "\n\n");

        if (processor->provider == DP_PROVIDER_GOOGLE_GEMINI) {
            if (event_end) {
                fprintf(stderr, "[DEBUG-GEMINI-SSE-SEPARATOR] Found '\\n\\n' at offset %ld from current_event_start.\n", event_end - current_event_start);
            } else {
                fprintf(stderr, "[DEBUG-GEMINI-SSE-SEPARATOR] Did NOT find '\\n\\n'. Will break from SSE processing loop.\n");
            }
            fflush(stderr);
        }

        if (!event_end) {
            // No complete SSE event found in the current buffer segment
            break; 
        }

        size_t event_len = event_end - current_event_start;
        char* event_data_segment = malloc(event_len + 1);
        if (!event_data_segment) { 
            const char* err_msg = "Event segment memory allocation failed";
             processor->user_callback(NULL, processor->user_data, true, err_msg);
            if (!processor->accumulated_error_during_stream) processor->accumulated_error_during_stream = dp_internal_strdup(err_msg);
            processor->stop_streaming_signal = true; 
            break; 
        }
        strncpy(event_data_segment, current_event_start, event_len);
        event_data_segment[event_len] = '\0';
        
        // Advance pointers for next iteration or buffer shifting
        current_event_start = event_end + 2; // Move past the processed event and its "\n\n"
        remaining_in_buffer -= (event_len + 2);

        char* line = event_data_segment;
        char* extracted_token_str = NULL; 
        bool is_final_for_this_event = false;

        while (line && *line) { // Process each line within the SSE event (e.g., "data: ...", "id: ...")
            char* next_line = strchr(line, '\n');
            if (next_line) *next_line = '\0'; // Null-terminate current line for processing

            if (strncmp(line, "data: ", 6) == 0) {
                char* json_str = line + 6;

                if (processor->provider == DP_PROVIDER_GOOGLE_GEMINI) {
                    fprintf(stderr, "[DEBUG-GEMINI-RAW-JSON-LINE]: <%s>\n", json_str);
                    fflush(stderr);
                }

                if (processor->provider == DP_PROVIDER_OPENAI_COMPATIBLE && strcmp(json_str, "[DONE]") == 0) {
                    is_final_for_this_event = true;
                    if (!processor->finish_reason_capture) processor->finish_reason_capture = dp_internal_strdup("done_marker");
                    break; // Break from inner while loop (processing lines of current event)
                }

                cJSON *json_chunk = cJSON_Parse(json_str);
                if (!json_chunk) {
                    if (processor->provider == DP_PROVIDER_GOOGLE_GEMINI) {
                        const char *error_ptr = cJSON_GetErrorPtr();
                        fprintf(stderr, "[DEBUG-GEMINI-PARSE-ERROR] Failed to parse JSON string from 'data:' line. Error near: %s. JSON was: <%s>\n", error_ptr ? error_ptr : "unknown", json_str);
                        fflush(stderr);
                    }
                } else {
                    // ... (cJSON parsing logic as in disasterparty_c_v8, with debug prints) ...
                    if (processor->provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
                        // ... (OpenAI parsing) ...
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
                        fprintf(stderr, "[DEBUG-GEMINI-PARSE] Successfully parsed JSON chunk from 'data:' line.\n");
                        cJSON *candidates = cJSON_GetObjectItemCaseSensitive(json_chunk, "candidates");
                        if (!cJSON_IsArray(candidates) || cJSON_GetArraySize(candidates) == 0) {
                            fprintf(stderr, "[DEBUG-GEMINI-PARSE] 'candidates' array not found or empty in JSON chunk.\n");
                        } else {
                            cJSON *candidate = cJSON_GetArrayItem(candidates, 0);
                            if (!candidate) fprintf(stderr, "[DEBUG-GEMINI-PARSE] First candidate is NULL in JSON chunk.\n");
                            else {
                                cJSON *content = cJSON_GetObjectItemCaseSensitive(candidate, "content");
                                if (!content) fprintf(stderr, "[DEBUG-GEMINI-PARSE] 'content' in candidate not found.\n");
                                else {
                                    cJSON *parts = cJSON_GetObjectItemCaseSensitive(content, "parts");
                                    if (!cJSON_IsArray(parts)) { 
                                        fprintf(stderr, "[DEBUG-GEMINI-PARSE] 'parts' in content not found or not an array.\n");
                                    } else {
                                        fprintf(stderr, "[DEBUG-GEMINI-PARSE] Found 'parts' array, size: %d\n", cJSON_GetArraySize(parts));
                                        cJSON* part_item_iter = NULL;
                                        char* current_event_accumulated_text = NULL; 

                                        cJSON_ArrayForEach(part_item_iter, parts) { 
                                            if(part_item_iter){
                                                cJSON *text = cJSON_GetObjectItemCaseSensitive(part_item_iter, "text");
                                                if (cJSON_IsString(text) && text->valuestring) {
                                                    fprintf(stderr, "[DEBUG-GEMINI-PARSE] Extracted text from a part: '%s' (len: %zu)\n", text->valuestring, strlen(text->valuestring));
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
                                                                fprintf(stderr, "[DEBUG-GEMINI-PARSE-ERROR] Malloc failed for concatenating token.\n");
                                                            }
                                                        }
                                                    }
                                                } else {
                                                    fprintf(stderr, "[DEBUG-GEMINI-PARSE] 'text' field not found or not a string in a part.\n");
                                                }
                                            }
                                        }
                                        if (current_event_accumulated_text) {
                                            // This token is for the current SSE event being processed
                                            free(extracted_token_str); // Free previous if any from same event (should not happen with current logic)
                                            extracted_token_str = current_event_accumulated_text; 
                                            fprintf(stderr, "[DEBUG-GEMINI-PARSE] Final accumulated token for this event: '%s'\n", extracted_token_str);
                                        } else {
                                            fprintf(stderr, "[DEBUG-GEMINI-PARSE] No text accumulated from parts for this event.\n");
                                        }
                                    }
                                }
                                cJSON *reason_cand = cJSON_GetObjectItemCaseSensitive(candidate, "finishReason");
                                if (cJSON_IsString(reason_cand) && reason_cand->valuestring) {
                                    fprintf(stderr, "[DEBUG-GEMINI-PARSE] Candidate finishReason: '%s'\n", reason_cand->valuestring);
                                    if (!processor->finish_reason_capture) processor->finish_reason_capture = dp_internal_strdup(reason_cand->valuestring);
                                    is_final_for_this_event = true;
                                }
                            }
                        }
                        cJSON *prompt_feedback = cJSON_GetObjectItemCaseSensitive(json_chunk, "promptFeedback");
                        if (prompt_feedback) {
                            cJSON *reason_pf = cJSON_GetObjectItemCaseSensitive(prompt_feedback, "finishReason");
                            if (cJSON_IsString(reason_pf) && reason_pf->valuestring) {
                                fprintf(stderr, "[DEBUG-GEMINI-PARSE] PromptFeedback finishReason: '%s'\n", reason_pf->valuestring);
                                if (!processor->finish_reason_capture) processor->finish_reason_capture = dp_internal_strdup(reason_pf->valuestring);
                                is_final_for_this_event = true;
                            }
                        }
                        fflush(stderr); 
                    }
                    cJSON_Delete(json_chunk);
                } 
            } // End "data: " line processing
            if (next_line) line = next_line + 1; else break; // Move to next line in event_data_segment
        } // End while loop for lines in event_data_segment
        free(event_data_segment);

        // Call user callback if a token was extracted for this event, or if it's a final event marker
        if (extracted_token_str) {
            if (processor->provider == DP_PROVIDER_GOOGLE_GEMINI) {
                 fprintf(stderr, "[DEBUG-GEMINI-CALLBACK] Calling user_callback with token: '%s', is_final: %d\n", extracted_token_str, is_final_for_this_event);
                 fflush(stderr);
            }
            if (processor->user_callback(extracted_token_str, processor->user_data, is_final_for_this_event, NULL) != 0) {
                processor->stop_streaming_signal = true;
            }
            free(extracted_token_str);
            extracted_token_str = NULL; // Reset for next potential event in buffer
        } else if (is_final_for_this_event) { // E.g. [DONE] marker or finish_reason without text
            if (processor->provider == DP_PROVIDER_GOOGLE_GEMINI) {
                fprintf(stderr, "[DEBUG-GEMINI-CALLBACK] Calling user_callback with NULL token (final event marker).\n");
                fflush(stderr);
            }
            if (processor->user_callback(NULL, processor->user_data, true, NULL) != 0) {
                processor->stop_streaming_signal = true;
            }
        }
        if (is_final_for_this_event) processor->stop_streaming_signal = true; // Stop processing further SSE events
    } // End while(true) loop for processing events in buffer

    // Shift unprocessed data to the beginning of the buffer
    if (remaining_in_buffer > 0 && current_event_start > processor->buffer) {
        memmove(processor->buffer, current_event_start, remaining_in_buffer);
    }
    processor->buffer_size = remaining_in_buffer;
    if (processor->buffer_size < processor->buffer_capacity) { // Ensure null termination
        processor->buffer[processor->buffer_size] = '\0'; 
    }
    return realsize;
}

int dp_perform_completion(dp_context_t* context, const dp_request_config_t* request_config, dp_response_t* response) {
    if (!context || !request_config || !response) {
        if (response) response->error_message = dp_internal_strdup("Invalid arguments to dp_perform_completion.");
        return -1;
    }
    if (request_config->stream) {
        if (response) response->error_message = dp_internal_strdup("dp_perform_completion called with stream=true. Use dp_perform_streaming_completion instead.");
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
    } else { 
        snprintf(url, sizeof(url), "%s/models/%s:generateContent?key=%s",
                 context->api_base_url, request_config->model, context->api_key);
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
                    if (error_obj) {
                        cJSON* msg_item = cJSON_GetObjectItemCaseSensitive(error_obj, "message");
                        if (cJSON_IsString(msg_item) && msg_item->valuestring) {
                             asprintf(&response->error_message, "API error (HTTP %ld): %s", response->http_status_code, msg_item->valuestring);
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
    if (context->provider == DP_PROVIDER_OPENAI_COMPATIBLE && !request_config->stream) {
         if (response) response->error_message = dp_internal_strdup("dp_perform_streaming_completion for OpenAI requires stream=true in config.");
        return -1;
    }
    memset(response, 0, sizeof(dp_response_t));

    CURL* curl = curl_easy_init();
    if (!curl) {
        response->error_message = dp_internal_strdup("curl_easy_init() failed for Disaster Party streaming.");
        return -1;
    }

    stream_processor_t processor = {0};
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
    } else { 
        snprintf(url, sizeof(url), "%s/models/%s:streamGenerateContent?key=%s&alt=sse",
                 context->api_base_url, request_config->model, context->api_key);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload_str);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, streaming_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&processor);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, DISASTERPARTY_USER_AGENT);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_status_code);

    if (!processor.stop_streaming_signal) { 
        const char* final_stream_error = processor.accumulated_error_during_stream;
        if (res != CURLE_OK && !final_stream_error) { 
            final_stream_error = curl_easy_strerror(res);
        } else if ((response->http_status_code < 200 || response->http_status_code >= 300) && !final_stream_error) { 
            cJSON* error_json = cJSON_Parse(processor.buffer);
            if (error_json) {
                cJSON* error_obj = cJSON_GetObjectItemCaseSensitive(error_json, "error");
                if (error_obj) {
                    cJSON* msg_item = cJSON_GetObjectItemCaseSensitive(error_obj, "message");
                    if (cJSON_IsString(msg_item) && msg_item->valuestring) {
                        final_stream_error = msg_item->valuestring; 
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

