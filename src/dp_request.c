#define _GNU_SOURCE 
#include "disasterparty.h"
#include "dp_private.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

// Memory callback for cURL operations - moved from dp_utils.c
size_t dpinternal_write_memory_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    memory_struct_t* mem = (memory_struct_t*)userp;
    char* ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        fprintf(stderr, "dpinternal_write_memory_callback: not enough memory (realloc returned NULL)\n");
        return 0;
    }
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

// HTTP request handling functions moved from disasterparty.c

int dp_perform_completion(dp_context_t* context, const dp_request_config_t* request_config, dp_response_t* response) {
    if (!context || !request_config || !response) {
        if (response) response->error_message = dpinternal_strdup("Invalid arguments to dp_perform_completion.");
        return -1;
    }
    if (request_config->stream) {
        if (response) response->error_message = dpinternal_strdup("dp_perform_completion called with stream=true. Use streaming functions instead.");
        return -1;
    }
    memset(response, 0, sizeof(dp_response_t));

    CURL* curl = curl_easy_init();
    if (!curl) {
        response->error_message = dpinternal_strdup("curl_easy_init() failed for Disaster Party completion.");
        return -1;
    }

    char* json_payload_str = NULL;
    if (context->provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
        json_payload_str = dpinternal_build_openai_json_payload_with_cjson(request_config, context);
    } else if (context->provider == DP_PROVIDER_GOOGLE_GEMINI) {
        json_payload_str = dpinternal_build_gemini_json_payload_with_cjson(request_config);
    } else if (context->provider == DP_PROVIDER_ANTHROPIC) {
        json_payload_str = dpinternal_build_anthropic_json_payload_with_cjson(request_config);
    }
    
    if (!json_payload_str) {
        response->error_message = dpinternal_strdup("Failed to build JSON payload for Disaster Party.");
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
        response->error_message = dpinternal_strdup("Memory allocation for response chunk failed.");
        free(json_payload_str); curl_slist_free_all(headers); curl_easy_cleanup(curl); return -1; 
    }
    chunk_mem.memory[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dpinternal_write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk_mem);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, context->user_agent);

    CURLcode res;
    if (context->provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
        res = dpinternal_perform_openai_request_with_fallback(curl, context, request_config, &chunk_mem, &response->http_status_code);
    } else {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload_str);
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_status_code);
    }

    if (res != CURLE_OK) {
        dpinternal_safe_asprintf(&response->error_message, "curl_easy_perform() failed: %s (HTTP status: %ld)",
                 curl_easy_strerror(res), response->http_status_code);
    } else {
        if (response->http_status_code >= 200 && response->http_status_code < 300) {
            char* extracted_text = dpinternal_extract_text_from_full_response_with_cjson(chunk_mem.memory, context->provider, &response->finish_reason);
            if (extracted_text) {
                response->parts = calloc(1, sizeof(dp_response_part_t));
                if (response->parts) {
                    response->num_parts = 1;
                    response->parts[0].type = DP_CONTENT_PART_TEXT;
                    response->parts[0].text = extracted_text; 
                } else {
                    free(extracted_text); 
                    response->error_message = dpinternal_strdup("Failed to allocate memory for response part structure.");
                }
            } else { 
                cJSON* error_root = cJSON_Parse(chunk_mem.memory);
                if (error_root) {
                    cJSON* error_obj = cJSON_GetObjectItemCaseSensitive(error_root, "error");
                    if (error_obj) { 
                        cJSON* msg_item = cJSON_GetObjectItemCaseSensitive(error_obj, "message");
                        if (cJSON_IsString(msg_item) && msg_item->valuestring) {
                             dpinternal_safe_asprintf(&response->error_message, "API error (HTTP %ld): %s", response->http_status_code, msg_item->valuestring);
                        }
                    } else { 
                         cJSON* type_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_root, "type");
                         cJSON* msg_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_root, "message"); 
                         if(cJSON_IsString(type_item_anthropic) && strcmp(type_item_anthropic->valuestring, "error") == 0 &&
                            cJSON_IsString(msg_item_anthropic) && msg_item_anthropic->valuestring) {
                            dpinternal_safe_asprintf(&response->error_message, "API error (HTTP %ld): %s", response->http_status_code, msg_item_anthropic->valuestring);
                         }
                    }
                    cJSON_Delete(error_root);
                }
                if (!response->error_message && chunk_mem.memory) { 
                    dpinternal_safe_asprintf(&response->error_message, "Failed to parse successful response or extract text (HTTP %ld). Body: %.200s...", response->http_status_code, chunk_mem.memory);
                } else if (!response->error_message) {
                    dpinternal_safe_asprintf(&response->error_message, "Failed to parse successful response or extract text (HTTP %ld). Empty response body.", response->http_status_code);
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
                dpinternal_safe_asprintf(&response->error_message, "HTTP error %ld: %s", response->http_status_code, api_err_detail);
            } else if (chunk_mem.memory) {
                dpinternal_safe_asprintf(&response->error_message, "HTTP error %ld. Body: %.500s", response->http_status_code, chunk_mem.memory);
            } else {
                dpinternal_safe_asprintf(&response->error_message, "HTTP error %ld. (no response body)", response->http_status_code);
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
        if (response) response->error_message = dpinternal_strdup("Invalid arguments to dp_perform_streaming_completion.");
        return -1;
    }
    
    if (context->provider != DP_PROVIDER_GOOGLE_GEMINI && !request_config->stream) {
         if (response) response->error_message = dpinternal_strdup("dp_perform_streaming_completion requires stream=true in config for OpenAI and Anthropic.");
        return -1;
    }

    memset(response, 0, sizeof(dp_response_t));

    CURL* curl = curl_easy_init();
    if (!curl) {
        response->error_message = dpinternal_strdup("curl_easy_init() failed for Disaster Party streaming.");
        return -1;
    }

    stream_processor_t processor = {0}; 
    processor.user_callback = callback;
    processor.user_data = user_data;
    processor.provider = context->provider;
    processor.buffer_capacity = 8192; 
    processor.buffer = malloc(processor.buffer_capacity);
    if (!processor.buffer) { 
        response->error_message = dpinternal_strdup("Stream processor buffer alloc failed.");
        curl_easy_cleanup(curl); return -1; 
    }
    processor.buffer[0] = '\0';

    char* json_payload_str = NULL;
    if (context->provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
        json_payload_str = dpinternal_build_openai_json_payload_with_cjson(request_config, context); 
    } else if (context->provider == DP_PROVIDER_GOOGLE_GEMINI) {
        json_payload_str = dpinternal_build_gemini_json_payload_with_cjson(request_config);
    } else if (context->provider == DP_PROVIDER_ANTHROPIC) {
        json_payload_str = dpinternal_build_anthropic_json_payload_with_cjson(request_config);
    }

     if (!json_payload_str) { 
        response->error_message = dpinternal_strdup("Payload build failed for streaming.");
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
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dpinternal_streaming_write_callback); 
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&processor);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, context->user_agent);

    CURLcode res;
    if (context->provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
        res = dpinternal_perform_openai_streaming_request_with_fallback(curl, context, request_config, &processor, &response->http_status_code);
    } else {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload_str);
        res = curl_easy_perform(curl);
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_status_code);
    }

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
                           response->error_message = dpinternal_strdup(final_stream_error);
                        }
                    }
                } else { 
                     cJSON* type_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_json, "type");
                     cJSON* msg_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_json, "message"); 
                     if(cJSON_IsString(type_item_anthropic) && strcmp(type_item_anthropic->valuestring, "error") == 0 &&
                        cJSON_IsString(msg_item_anthropic) && msg_item_anthropic->valuestring) {
                        final_stream_error = msg_item_anthropic->valuestring;
                        if (response->error_message == NULL) { 
                           response->error_message = dpinternal_strdup(final_stream_error);
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
        response->finish_reason = dpinternal_strdup("completed");
    }

    if (res != CURLE_OK && !response->error_message) {
        dpinternal_safe_asprintf(&response->error_message, "curl_easy_perform() failed: %s", curl_easy_strerror(res));
    } else if ((response->http_status_code < 200 || response->http_status_code >= 300) && !response->error_message) {
         char* temp_err_msg = NULL;
         if (processor.buffer_size > 0) {
            cJSON* error_json = cJSON_Parse(processor.buffer);
            if (error_json) {
                cJSON* error_obj = cJSON_GetObjectItemCaseSensitive(error_json, "error");
                if (error_obj) {
                    cJSON* msg_item = cJSON_GetObjectItemCaseSensitive(error_obj, "message");
                    if (cJSON_IsString(msg_item) && msg_item->valuestring) {
                        dpinternal_safe_asprintf(&temp_err_msg, "HTTP error %ld: %s", response->http_status_code, msg_item->valuestring);
                    }
                } else {
                     cJSON* type_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_json, "type");
                     cJSON* msg_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_json, "message");
                     if(cJSON_IsString(type_item_anthropic) && strcmp(type_item_anthropic->valuestring, "error") == 0 &&
                        cJSON_IsString(msg_item_anthropic) && msg_item_anthropic->valuestring) {
                        dpinternal_safe_asprintf(&temp_err_msg, "HTTP error %ld: %s", response->http_status_code, msg_item_anthropic->valuestring);
                     }
                }
                cJSON_Delete(error_json);
            }
         }
         if (temp_err_msg) {
            response->error_message = temp_err_msg;
         } else if (processor.buffer_size > 0) {
            dpinternal_safe_asprintf(&response->error_message, "HTTP error %ld. Body hint: %.200s", response->http_status_code, processor.buffer);
         } else {
            dpinternal_safe_asprintf(&response->error_message, "HTTP error %ld (empty body)", response->http_status_code);
         }
    }

    if (processor.accumulated_error_during_stream) {
        if (response->error_message) {
             char* combined_error;
             if (strstr(response->error_message, processor.accumulated_error_during_stream) == NULL) {
                if (dpinternal_safe_asprintf(&combined_error, "%s; Stream processing error: %s", response->error_message, processor.accumulated_error_during_stream) != -1) {
                    free(response->error_message);
                    response->error_message = combined_error;
                }
             }
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
        if (response) response->error_message = dpinternal_strdup("Invalid arguments to dp_perform_anthropic_streaming_completion.");
        return -1;
    }
    if (context->provider != DP_PROVIDER_ANTHROPIC) {
        if (response) response->error_message = dpinternal_strdup("dp_perform_anthropic_streaming_completion called with non-Anthropic provider.");
        return -1;
    }
    if (!request_config->stream) {
         if (response) response->error_message = dpinternal_strdup("dp_perform_anthropic_streaming_completion requires stream=true in config.");
        return -1;
    }

    memset(response, 0, sizeof(dp_response_t));

    CURL* curl = curl_easy_init();
    if (!curl) {
        response->error_message = dpinternal_strdup("curl_easy_init() failed for Anthropic streaming.");
        return -1;
    }

    anthropic_stream_processor_t processor = {0}; 
    processor.anthropic_user_callback = anthropic_callback;
    processor.user_data = user_data;
    processor.buffer_capacity = 8192; 
    processor.buffer = malloc(processor.buffer_capacity);
    if (!processor.buffer) { 
        response->error_message = dpinternal_strdup("Anthropic stream processor buffer alloc failed.");
        curl_easy_cleanup(curl); return -1; 
    }
    processor.buffer[0] = '\0';

    char* json_payload_str = dpinternal_build_anthropic_json_payload_with_cjson(request_config);
    if (!json_payload_str) { 
        response->error_message = dpinternal_strdup("Payload build failed for Anthropic streaming.");
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
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dpinternal_anthropic_detailed_stream_write_callback); 
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&processor);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, context->user_agent);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_status_code);

    if (!processor.stop_streaming_signal) { 
        const char* final_stream_error = processor.accumulated_error_during_stream;
         dp_anthropic_stream_event_t final_event = { .event_type = DP_ANTHROPIC_EVENT_UNKNOWN, .raw_json_data = NULL};

        if (res != CURLE_OK && !final_stream_error) { 
            final_stream_error = curl_easy_strerror(res);
            final_event.event_type = DP_ANTHROPIC_EVENT_ERROR; 
            final_event.raw_json_data = final_stream_error; 
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
        response->finish_reason = dpinternal_strdup("completed");
    }
    
    if (res != CURLE_OK && !response->error_message) {
        dpinternal_safe_asprintf(&response->error_message, "curl_easy_perform() failed: %s", curl_easy_strerror(res));
    } else if ((response->http_status_code < 200 || response->http_status_code >= 300) && !response->error_message) {
         char* temp_err_msg = NULL;
         if (processor.buffer_size > 0) {
            cJSON* error_json = cJSON_Parse(processor.buffer);
            if (error_json) {
                cJSON* error_obj = cJSON_GetObjectItemCaseSensitive(error_json, "error");
                if (error_obj) {
                    cJSON* msg_item = cJSON_GetObjectItemCaseSensitive(error_obj, "message");
                    if (cJSON_IsString(msg_item) && msg_item->valuestring) {
                        dpinternal_safe_asprintf(&temp_err_msg, "HTTP error %ld: %s", response->http_status_code, msg_item->valuestring);
                    }
                } else {
                     cJSON* type_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_json, "type");
                     cJSON* msg_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_json, "message");
                     if(cJSON_IsString(type_item_anthropic) && strcmp(type_item_anthropic->valuestring, "error") == 0 &&
                        cJSON_IsString(msg_item_anthropic) && msg_item_anthropic->valuestring) {
                        dpinternal_safe_asprintf(&temp_err_msg, "HTTP error %ld: %s", response->http_status_code, msg_item_anthropic->valuestring);
                     }
                }
                cJSON_Delete(error_json);
            }
         }
         if (temp_err_msg) {
            response->error_message = temp_err_msg;
         } else if (processor.buffer_size > 0) {
            dpinternal_safe_asprintf(&response->error_message, "HTTP error %ld. Body hint: %.200s", response->http_status_code, processor.buffer);
         } else {
            dpinternal_safe_asprintf(&response->error_message, "HTTP error %ld (empty body)", response->http_status_code);
         }
    }

    if (processor.accumulated_error_during_stream) {
        if (response->error_message) {
             char* combined_error;
             if (strstr(response->error_message, processor.accumulated_error_during_stream) == NULL) {
                if (dpinternal_safe_asprintf(&combined_error, "%s; Stream processing error: %s", response->error_message, processor.accumulated_error_during_stream) != -1) {
                    free(response->error_message);
                    response->error_message = combined_error;
                }
             }
        } else {
            response->error_message = processor.accumulated_error_during_stream; 
        }
    }

    if (processor.accumulated_error_during_stream) { 
        free(processor.accumulated_error_during_stream); 
    }
    free(json_payload_str);
    free(processor.buffer);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response->error_message ? -1 : 0;
}