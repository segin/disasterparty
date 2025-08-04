#define _GNU_SOURCE 
#include "disasterparty.h"
#include "dp_private.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

// Helper function to perform OpenAI streaming request with automatic token parameter fallback
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
        dpinternal_is_token_parameter_error(processor->buffer, *http_status_code)) {
        
        // Switch to legacy parameter and retry
        context->token_param_preference = DP_TOKEN_PARAM_MAX_TOKENS;
        free(json_payload_str);
        
        // Reset processor buffer for retry
        if (processor->buffer) {
            processor->buffer[0] = '\0';
            processor->buffer_size = 0;
        }
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

size_t dpinternal_streaming_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    stream_processor_t* processor = (stream_processor_t*)userp;

    if (processor->stop_streaming_signal) {
        return realsize; 
    }

    size_t needed_capacity = processor->buffer_size + realsize + 1;
    if (processor->buffer_capacity < needed_capacity) {
        size_t new_capacity = needed_capacity > processor->buffer_capacity * 2 ? needed_capacity : processor->buffer_capacity * 2;
        if (new_capacity < 1024) new_capacity = 1024;
        char* new_buf = realloc(processor->buffer, new_capacity);
        if (!new_buf) {
            const char* err_msg = "Stream buffer memory re-allocation failed";
            processor->user_callback(NULL, processor->user_data, true, err_msg);
            if (!processor->accumulated_error_during_stream) processor->accumulated_error_during_stream = dpinternal_strdup(err_msg);
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
            const char* err_msg = "Event segment memory allocation failed";
             processor->user_callback(NULL, processor->user_data, true, err_msg);
            if (!processor->accumulated_error_during_stream) processor->accumulated_error_during_stream = dpinternal_strdup(err_msg);
            processor->stop_streaming_signal = true; 
            break; 
        }
        strncpy(event_data_segment, current_event_start, event_len);
        event_data_segment[event_len] = '\0';
        
        current_event_start = event_end + separator_len; 
        remaining_in_buffer -= (event_len + separator_len);

        char* line = event_data_segment;
        char* extracted_token_str = NULL; 
        bool is_final_for_this_event = false;
        char* sse_event_type_str = NULL;

        while (line && *line) { 
            char* next_line = strchr(line, '\n');
            if (next_line) {
                *next_line = '\0'; 
                if ((next_line > line) && (*(next_line - 1) == '\r')) { 
                    *(next_line - 1) = '\0';
                }
            }
            
            if (processor->provider == DP_PROVIDER_ANTHROPIC && strncmp(line, "event: ", 7) == 0) {
                free(sse_event_type_str); 
                sse_event_type_str = dpinternal_strdup(line + 7);
            } else if (strncmp(line, "data: ", 6) == 0) {
                char* json_str = line + 6;

                if (processor->provider == DP_PROVIDER_OPENAI_COMPATIBLE && strcmp(json_str, "[DONE]") == 0) {
                    is_final_for_this_event = true;
                    if (!processor->finish_reason_capture) processor->finish_reason_capture = dpinternal_strdup("done_marker");
                    break; 
                }

                cJSON *json_chunk = cJSON_Parse(json_str);
                if (json_chunk) {
                    if (processor->provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
                         cJSON *choices = cJSON_GetObjectItemCaseSensitive(json_chunk, "choices");
                        if (cJSON_IsArray(choices) && cJSON_GetArraySize(choices) > 0) {
                            cJSON *choice = cJSON_GetArrayItem(choices, 0);
                            if(choice) {
                                cJSON *delta = cJSON_GetObjectItemCaseSensitive(choice, "delta");
                                if (delta) {
                                    cJSON *content = cJSON_GetObjectItemCaseSensitive(delta, "content");
                                    if (cJSON_IsString(content) && content->valuestring && strlen(content->valuestring) > 0) {
                                        extracted_token_str = dpinternal_strdup(content->valuestring);
                                    }
                                }
                                if (!processor->finish_reason_capture) {
                                    cJSON *reason = cJSON_GetObjectItemCaseSensitive(choice, "finish_reason");
                                    if (cJSON_IsString(reason) && reason->valuestring) {
                                        processor->finish_reason_capture = dpinternal_strdup(reason->valuestring);
                                        is_final_for_this_event = true;
                                    }
                                }
                            }
                        }
                    } else if (processor->provider == DP_PROVIDER_GOOGLE_GEMINI) { 
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
                                                            current_event_accumulated_text = dpinternal_strdup(text->valuestring);
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
                                    if (!processor->finish_reason_capture) processor->finish_reason_capture = dpinternal_strdup(reason_cand->valuestring);
                                    is_final_for_this_event = true;
                                }
                            }
                        }
                        cJSON *prompt_feedback = cJSON_GetObjectItemCaseSensitive(json_chunk, "promptFeedback");
                        if (prompt_feedback) {
                            cJSON *reason_pf = cJSON_GetObjectItemCaseSensitive(prompt_feedback, "finishReason");
                            if (cJSON_IsString(reason_pf) && reason_pf->valuestring) {
                                if (!processor->finish_reason_capture) processor->finish_reason_capture = dpinternal_strdup(reason_pf->valuestring);
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
                                         extracted_token_str = dpinternal_strdup(text->valuestring);
                                    }
                                }
                            }
                        } else if (sse_event_type_str && strcmp(sse_event_type_str, "message_delta") == 0) {
                             if (!processor->finish_reason_capture) {
                                cJSON* usage = cJSON_GetObjectItemCaseSensitive(json_chunk, "usage");
                                if(usage){
                                    cJSON* stop_reason_item = cJSON_GetObjectItemCaseSensitive(usage, "stop_reason");
                                    if(cJSON_IsString(stop_reason_item) && stop_reason_item->valuestring){
                                         processor->finish_reason_capture = dpinternal_strdup(stop_reason_item->valuestring);
                                    }
                                } else { 
                                    cJSON* delta = cJSON_GetObjectItemCaseSensitive(json_chunk, "delta");
                                    if(delta) {
                                        cJSON* stop_reason_item = cJSON_GetObjectItemCaseSensitive(delta, "stop_reason");
                                        if(cJSON_IsString(stop_reason_item) && stop_reason_item->valuestring){
                                             processor->finish_reason_capture = dpinternal_strdup(stop_reason_item->valuestring);
                                        }
                                    }
                                }
                             }
                        } else if (sse_event_type_str && strcmp(sse_event_type_str, "message_stop") == 0) {
                            is_final_for_this_event = true;
                        } else if (sse_event_type_str && strcmp(sse_event_type_str, "error") == 0) {
                            cJSON* error_obj = cJSON_GetObjectItemCaseSensitive(json_chunk, "error");
                            if(error_obj) {
                                cJSON* err_type = cJSON_GetObjectItemCaseSensitive(error_obj, "type");
                                cJSON* err_msg = cJSON_GetObjectItemCaseSensitive(error_obj, "message");
                                if(cJSON_IsString(err_type) && cJSON_IsString(err_msg)){
                                     char* temp_err = NULL;
                                     if (dpinternal_safe_asprintf(&temp_err, "Anthropic Stream Error (%s): %s", err_type->valuestring, err_msg->valuestring) == -1) {
                                         temp_err = NULL;
                                     }
                                     if(!processor->accumulated_error_during_stream) processor->accumulated_error_during_stream = temp_err; else free(temp_err);
                                }
                            }
                            is_final_for_this_event = true; 
                        }
                    } 
                    cJSON_Delete(json_chunk);
                } 
            } 
            if (next_line) line = next_line + 1 + ((next_line > line && *(next_line-1) == '\r') ? -1 : 0) ; else break;
        } 
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
    }
    processor->buffer_size = remaining_in_buffer;
    if (processor->buffer_size < processor->buffer_capacity) { 
        processor->buffer[processor->buffer_size] = '\0'; 
    }
    return realsize;
}
size_t dpinternal_anthropic_detailed_stream_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    anthropic_stream_processor_t* processor = (anthropic_stream_processor_t*)userp;

    if (processor->stop_streaming_signal) return realsize;

    size_t needed_capacity = processor->buffer_size + realsize + 1;
    if (processor->buffer_capacity < needed_capacity) {
        size_t new_capacity = needed_capacity > processor->buffer_capacity * 2 ? needed_capacity : processor->buffer_capacity * 2;
        if (new_capacity < 1024) new_capacity = 1024;
        char* new_buf = realloc(processor->buffer, new_capacity);
        if (!new_buf) {
            dp_anthropic_stream_event_t event = { .event_type = DP_ANTHROPIC_EVENT_ERROR, .raw_json_data = "{\"error\":{\"type\":\"internal_error\",\"message\":\"Stream buffer memory re-allocation failed\"}}" };
            processor->anthropic_user_callback(&event, processor->user_data, "Stream buffer memory re-allocation failed");
            if (!processor->accumulated_error_during_stream) processor->accumulated_error_during_stream = dpinternal_strdup("Stream buffer memory re-allocation failed");
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
            dp_anthropic_stream_event_t event = { .event_type = DP_ANTHROPIC_EVENT_ERROR, .raw_json_data = "{\"error\":{\"type\":\"internal_error\",\"message\":\"Event segment memory allocation failed\"}}" };
            processor->anthropic_user_callback(&event, processor->user_data, "Event segment memory allocation failed");
            if(!processor->accumulated_error_during_stream) processor->accumulated_error_during_stream = dpinternal_strdup("Event segment memory allocation failed");
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
                free(temp_event_type_str); 
                temp_event_type_str = dpinternal_strdup(line + 7);
            } else if (strncmp(line, "data: ", 6) == 0) {
                free(temp_json_data_str); 
                temp_json_data_str = dpinternal_strdup(line + 6);
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
        current_api_event.raw_json_data = temp_json_data_str; 

        if (current_api_event.event_type == DP_ANTHROPIC_EVENT_MESSAGE_STOP || current_api_event.event_type == DP_ANTHROPIC_EVENT_ERROR) {
             if (!processor->finish_reason_capture && current_api_event.raw_json_data) {
                cJSON* data_json = cJSON_Parse(current_api_event.raw_json_data);
                if(data_json) {
                    if (current_api_event.event_type == DP_ANTHROPIC_EVENT_MESSAGE_STOP) {
                        processor->finish_reason_capture = dpinternal_strdup("message_stop_event");
                    } else { // DP_ANTHROPIC_EVENT_ERROR
                        cJSON* error_obj = cJSON_GetObjectItemCaseSensitive(data_json, "error");
                        if(error_obj) {
                            cJSON* type_item = cJSON_GetObjectItemCaseSensitive(error_obj, "type");
                            if(cJSON_IsString(type_item)) {
                                processor->finish_reason_capture = dpinternal_strdup(type_item->valuestring);
                            } else {
                                processor->finish_reason_capture = dpinternal_strdup("error_event");
                            }
                        }
                    }
                    cJSON_Delete(data_json);
                }
             }
             processor->stop_streaming_signal = true; 
        }
        
        if (processor->anthropic_user_callback(&current_api_event, processor->user_data, NULL) != 0) {
            processor->stop_streaming_signal = true;
        }
        free(temp_json_data_str); 
        free(event_data_segment);
    } 

    if (remaining_in_buffer > 0 && current_event_start < processor->buffer + processor->buffer_size) {
        memmove(processor->buffer, current_event_start, remaining_in_buffer);
    } else if (remaining_in_buffer == 0) {
        processor->buffer_size = 0;
        if (processor->buffer_capacity > 0) {
            processor->buffer[0] = '\0';
        }
    }
    processor->buffer_size = remaining_in_buffer;
    return realsize;
}