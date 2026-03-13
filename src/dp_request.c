#define _GNU_SOURCE 
#include "disasterparty.h"
#include "dp_private.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>

// Memory callback for cURL operations - moved from disasterparty.c to be used globally
size_t dpinternal_write_memory_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    memory_struct_t* mem = (memory_struct_t*)userp;

    char* ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

int dp_perform_completion(dp_context_t* context,
                          const dp_request_config_t* request_config,
                          dp_response_t* response) {
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
            bool parse_success = dpinternal_parse_response_content(context, chunk_mem.memory, &response->parts, &response->num_parts, &response->finish_reason);

            if (parse_success && response->num_parts > 0) {
                // Success
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
                    cJSON* msg_item_anthropic = cJSON_GetObjectItemCaseSensitive(error_root, "message");
                    if(cJSON_IsString(msg_item_anthropic) && msg_item_anthropic->valuestring){
                        api_err_detail = msg_item_anthropic->valuestring;
                    }
                }
             }
             if (api_err_detail) {
                 dpinternal_safe_asprintf(&response->error_message, "API returned error (HTTP %ld): %s", response->http_status_code, api_err_detail);
             } else {
                 dpinternal_safe_asprintf(&response->error_message, "API request failed with HTTP status %ld. Body: %.200s...", response->http_status_code, chunk_mem.memory ? chunk_mem.memory : "(empty)");
             }
             if(error_root) cJSON_Delete(error_root);
        }
    }

    free(json_payload_str);
    if (chunk_mem.memory) free(chunk_mem.memory);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response->error_message ? -1 : 0;
}

int dp_perform_streaming_completion(dp_context_t* context,
                                    const dp_request_config_t* request_config,
                                    dp_stream_callback_t callback, 
                                    void* user_data,
                                    dp_response_t* response) {
    if (!context || !request_config || !callback || !response) {
        if (response) response->error_message = dpinternal_strdup("Invalid arguments to dp_perform_streaming_completion.");
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
    processor.features = context->features;
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

    if (processor.finish_reason_capture) response->finish_reason = processor.finish_reason_capture;
    if (res != CURLE_OK && !response->error_message) response->error_message = dpinternal_strdup(curl_easy_strerror(res));

    free(json_payload_str);
    free(processor.buffer);
    if (processor.accumulated_error_during_stream) { 
        free(processor.accumulated_error_during_stream); 
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response->error_message ? -1 : 0;
}

int dp_perform_detailed_streaming_completion(dp_context_t* context,
                                              const dp_request_config_t* request_config,
                                              dp_detailed_stream_callback_t callback,
                                              void* user_data,
                                              dp_response_t* response) {
    if (!context || !request_config || !callback || !response) {
        if (response) response->error_message = dpinternal_strdup("Invalid arguments to dp_perform_detailed_streaming_completion.");
        return -1;
    }

    memset(response, 0, sizeof(dp_response_t));

    CURL* curl = curl_easy_init();
    if (!curl) {
        response->error_message = dpinternal_strdup("curl_easy_init() failed for detailed streaming.");
        return -1;
    }

    stream_processor_t processor = {0}; 
    processor.detailed_callback = callback;
    processor.user_data = user_data;
    processor.provider = context->provider;
    processor.buffer_capacity = 8192; 
    processor.features = context->features;
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
        response->error_message = dpinternal_strdup("Payload build failed for detailed streaming.");
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

    // For Anthropic, we still use the specialized SSE parser because it has unique events
    if (context->provider == DP_PROVIDER_ANTHROPIC) {
        anthropic_stream_processor_t anthro_processor = {0};
        anthro_processor.anthropic_user_callback = callback;
        anthro_processor.user_data = user_data;
        anthro_processor.buffer_capacity = 8192;
        anthro_processor.buffer = malloc(anthro_processor.buffer_capacity);
        if (!anthro_processor.buffer) { 
            response->error_message = dpinternal_strdup("Anthro processor buffer alloc failed.");
            free(json_payload_str); curl_slist_free_all(headers); curl_easy_cleanup(curl); return -1; 
        }
        anthro_processor.buffer[0] = '\0';

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dpinternal_anthropic_detailed_stream_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&anthro_processor);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, context->user_agent);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload_str);

        CURLcode res;
        if (context->provider == DP_PROVIDER_ANTHROPIC) {
             res = curl_easy_perform(curl);
        } else {
             // Should not happen with current logic but for safety:
             res = CURLE_FAILED_INIT;
        }
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_status_code);

        if (anthro_processor.finish_reason_capture) response->finish_reason = anthro_processor.finish_reason_capture;
        
        free(anthro_processor.buffer);
        if (res != CURLE_OK && !response->error_message) response->error_message = dpinternal_strdup(curl_easy_strerror(res));
    } else {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dpinternal_streaming_write_callback); 
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&processor);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, context->user_agent);

        CURLcode res;
        if (context->provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
            res = dpinternal_perform_openai_detailed_streaming_request_with_fallback(curl, context, request_config, (anthropic_stream_processor_t*)&processor, &response->http_status_code);
        } else {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload_str);
            res = curl_easy_perform(curl);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_status_code);
        }

        if (processor.finish_reason_capture) response->finish_reason = processor.finish_reason_capture;
        if (res != CURLE_OK && !response->error_message) response->error_message = dpinternal_strdup(curl_easy_strerror(res));
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
    return dp_perform_detailed_streaming_completion(context, request_config, anthropic_callback, user_data, response);
}

int dp_generate_image(dp_context_t* context, const dp_image_generation_config_t* config, dp_image_generation_response_t* response) {
    if (!context || !config || !response) return -1;
    memset(response, 0, sizeof(dp_image_generation_response_t));

    CURL* curl = curl_easy_init();
    if (!curl) return -1;

    char* json_payload = NULL;
    char url[1024];

    if (context->provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
        json_payload = dpinternal_build_openai_image_generation_payload_with_cjson(config);
        snprintf(url, sizeof(url), "%s/images/generations", context->api_base_url);
    } else if (context->provider == DP_PROVIDER_GOOGLE_GEMINI) {
        json_payload = dpinternal_build_google_image_generation_payload_with_cjson(config, context);
        // Gemini image generation URL is specific to the model and project, simplified here.
        snprintf(url, sizeof(url), "%s/v1/projects/%s/locations/us-central1/publishers/google/models/%s:predict",
                 context->api_base_url, "PROJECT_ID", config->model ? config->model : "imagen-3.0-generate-001");
    }

    if (!json_payload) {
        curl_easy_cleanup(curl);
        return -1;
    }

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    if (context->provider == DP_PROVIDER_OPENAI_COMPATIBLE) {
        char auth_header[512];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", context->api_key);
        headers = curl_slist_append(headers, auth_header);
    } else if (context->provider == DP_PROVIDER_GOOGLE_GEMINI) {
        char auth_header[512];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", context->api_key);
        headers = curl_slist_append(headers, auth_header);
    }

    memory_struct_t chunk = { .memory = malloc(1), .size = 0 };
    chunk.memory[0] = '\0';

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dpinternal_write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, context->user_agent);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->http_status_code);

    if (res == CURLE_OK && response->http_status_code == 200) {
        // Parse image response... (Simplified)
    } else {
        response->error_message = dpinternal_strdup(curl_easy_strerror(res));
    }

    free(json_payload);
    free(chunk.memory);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response->error_message ? -1 : 0;
}
