#define _GNU_SOURCE
#include "dp_private.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

size_t streaming_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
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
            if (!processor->accumulated_error_during_stream) processor->accumulated_error_during_stream = dp_internal_strdup(err_msg);
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
            if (!processor->accumulated_error_during_stream) processor->accumulated_error_during_stream = dp_internal_strdup(err_msg);
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
                                cJSON* usage = cJSON_GetObjectItemCaseSensitive(json_chunk, "usage");
                                if(usage){
                                    cJSON* stop_reason_item = cJSON_GetObjectItemCaseSensitive(usage, "stop_reason");
                                    if(cJSON_IsString(stop_reason_item) && stop_reason_item->valuestring){
                                         processor->finish_reason_capture = dp_internal_strdup(stop_reason_item->valuestring);
                                    }
                                } else { 
                                    cJSON* delta = cJSON_GetObjectItemCaseSensitive(json_chunk, "delta");
                                    if(delta) {
                                        cJSON* stop_reason_item = cJSON_GetObjectItemCaseSensitive(delta, "stop_reason");
                                        if(cJSON_IsString(stop_reason_item) && stop_reason_item->valuestring){
                                             processor->finish_reason_capture = dp_internal_strdup(stop_reason_item->valuestring);
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
                                     char* temp_err;
                                     asprintf(&temp_err, "Anthropic Stream Error (%s): %s", err_type->valuestring, err_msg->valuestring);
                                     if(!processor->accumulated_error_during_stream) processor->accumulated_error_during_stream = temp_err; else free(temp_err);
                                }
                            }
                            is_final_for_this_event = true; 
                        }
                    } 
                    cJSON_Delete(json_chunk);
                } 
            } 
            if (next_line) line = next_line + 1; else break;
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


size_t anthropic_detailed_stream_write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
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
        }
        else {
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
                free(temp_event_type_str); 
                temp_event_type_str = dp_internal_strdup(line + 7);
            } else if (strncmp(line, "data: ", 6) == 0) {
                free(temp_json_data_str); 
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
        current_api_event.raw_json_data = temp_json_data_str; 

        if (current_api_event.event_type == DP_ANTHROPIC_EVENT_MESSAGE_STOP || current_api_event.event_type == DP_ANTHROPIC_EVENT_ERROR) {
             if (!processor->finish_reason_capture && current_api_event.raw_json_data) {
                cJSON* data_json = cJSON_Parse(current_api_event.raw_json_data);
                if(data_json) {
                    if (current_api_event.event_type == DP_ANTHROPIC_EVENT_MESSAGE_STOP) {
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
    }
    processor->buffer_size = remaining_in_buffer; 
    if (processor->buffer_size < processor->buffer_capacity) { 
        processor->buffer[processor->buffer_size] = '\0'; 
    }
    return realsize;
}


int dp_perform_streaming_completion(dp_context_t* context, const dp_request_config_t* request_config,
                                    dp_stream_callback_t callback, void* user_data, dp_response_t* response) {
    if (!context || !request_config || !callback || !response) {
        if (response) response->error_message = dp_internal_strdup("Invalid arguments to dp_perform_streaming_completion.");
        return -1;
    }
    
    if (context->provider != DP_PROVIDER_GOOGLE_GEMINI && !request_config->stream) {
         if (response) response->error_message = dp_internal_strdup("dp_perform_streaming_completion requires stream=true in config for OpenAI and Anthropic.");
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
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, streaming_write_callback); 
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&processor);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, dp_get_user_agent(context));

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
                        final_stream_error = (char*)msg_item->valuestring; 
                        if (response->error_message == NULL) { 
                           response->error_message = dp_internal_strdup(final_stream_error);
                        }
                    }
                } else { 
                     cJSON* type_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_json, "type");
                     cJSON* msg_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_json, "message"); 
                     if(cJSON_IsString(type_item_anthropic) && strcmp(type_item_anthropic->valuestring, "error") == 0 &&
                        cJSON_IsString(msg_item_anthropic) && msg_item_anthropic->valuestring) {
                        final_stream_error = (char*)msg_item_anthropic->valuestring;
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
                else {
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

    anthropic_stream_processor_t processor = {0}; 
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
    headers = curl_slist_append(headers, "anthropic-version: 2023-06-01"); 

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload_str);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, anthropic_detailed_stream_write_callback); 
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&processor);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, dp_get_user_agent(context));

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_status_code);

    if (!processor.stop_streaming_signal) { 
        const char* final_stream_error = processor.accumulated_error_during_stream;
         dp_anthropic_stream_event_t final_event = { .event_type = DP_ANTHROPIC_EVENT_UNKNOWN, .raw_json_data = NULL};

        if (res != CURLE_OK && !final_stream_error) { 
            final_stream_error = curl_easy_strerror(res);
            final_event.event_type = DP_ANTHROPIC_EVENT_ERROR; 
            final_event.raw_json_data = (char*)final_stream_error; 
        } else if ((response->http_status_code < 200 || response->http_status_code >= 300) && !final_stream_error) {
            final_event.event_type = DP_ANTHROPIC_EVENT_ERROR;
            final_event.raw_json_data = processor.buffer_size > 0 ? processor.buffer : "{\"error\":{\"type\":\"http_error\",\"message\":\"HTTP error occurred during stream\"}}";
            final_stream_error = processor.buffer_size > 0 ? processor.buffer : "HTTP error occurred during stream";
        } else if (!final_stream_error) { 
            final_event.event_type = DP_ANTHROPIC_EVENT_MESSAGE_STOP; 
        }
        
        if (final_event.event_type != DP_ANTHROPIC_EVENT_UNKNOWN || final_stream_error) {
            processor.anthropic_user_callback(&final_event, processor.user_data, final_stream_error);
        }
    }
    
    if (processor.finish_reason_capture) {
        response->finish_reason = processor.finish_reason_capture; 
    } else if (res == CURLE_OK && response->http_status_code >= 200 && response->http_status_code < 300 && !processor.accumulated_error_during_stream && !response->finish_reason) {
        response->finish_reason = dp_internal_strdup("completed");
    }
    
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