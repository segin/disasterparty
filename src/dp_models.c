#define _GNU_SOURCE
#include "dp_private.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
        (*model_list_out)->error_message = dpinternal_strdup("curl_easy_init() failed for list_models.");
        return -1;
    }

    char url[1024];
    struct curl_slist* headers = NULL;
    memory_struct_t chunk_mem = { .memory = malloc(1), .size = 0 };
    if (!chunk_mem.memory) {
        (*model_list_out)->error_message = dpinternal_strdup("Memory allocation for list_models response chunk failed.");
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
        snprintf(url, sizeof(url), "%s/models", context->api_base_url); 
        char api_key_header[512];
        snprintf(api_key_header, sizeof(api_key_header), "x-api-key: %s", context->api_key);
        headers = curl_slist_append(headers, api_key_header);
        headers = curl_slist_append(headers, "anthropic-version: 2023-06-01");
    } else {
        (*model_list_out)->error_message = dpinternal_strdup("Unsupported provider for list_models.");
        free(chunk_mem.memory);
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L); 
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dpinternal_write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk_mem);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, context->user_agent);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(*model_list_out)->http_status_code);

    int return_code = 0;

    if (res != CURLE_OK) {
        dpinternal_safe_asprintf(&(*model_list_out)->error_message, "curl_easy_perform() failed for list_models: %s (HTTP status: %ld)",
                 curl_easy_strerror(res), (*model_list_out)->http_status_code);
        return_code = -1;
    } else {
        if ((*model_list_out)->http_status_code >= 200 && (*model_list_out)->http_status_code < 300) {
            cJSON *root = cJSON_Parse(chunk_mem.memory);
            if (!root) {
                dpinternal_safe_asprintf(&(*model_list_out)->error_message, "Failed to parse JSON response for list_models. Body: %.200s...", chunk_mem.memory ? chunk_mem.memory : "(empty)");
                return_code = -1;
            } else {
                cJSON *data_array = NULL;
                if (context->provider == DP_PROVIDER_OPENAI_COMPATIBLE || context->provider == DP_PROVIDER_ANTHROPIC) {
                    data_array = cJSON_GetObjectItemCaseSensitive(root, "data");
                } else if (context->provider == DP_PROVIDER_GOOGLE_GEMINI) {
                    data_array = cJSON_GetObjectItemCaseSensitive(root, "models");
                }

                if (cJSON_IsArray(data_array)) {
                    int array_size = cJSON_GetArraySize(data_array);
                    (*model_list_out)->models = calloc(array_size, sizeof(dp_model_info_t));
                    if (!(*model_list_out)->models) {
                        (*model_list_out)->error_message = dpinternal_strdup("Failed to allocate memory for model info array.");
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
                            } else { 
                                id_item = cJSON_GetObjectItemCaseSensitive(model_json, "name");
                            }
                            if (cJSON_IsString(id_item) && id_item->valuestring) {
                                const char* model_name_str = id_item->valuestring;
                                if (context->provider == DP_PROVIDER_GOOGLE_GEMINI && strncmp(model_name_str, "models/", 7) == 0) {
                                    model_name_str += 7; 
                                }
                                info->model_id = dpinternal_strdup(model_name_str);
                            }

                            cJSON *display_name_item = cJSON_GetObjectItemCaseSensitive(model_json, "displayName"); 
                             if (!display_name_item && context->provider == DP_PROVIDER_ANTHROPIC) { 
                                display_name_item = cJSON_GetObjectItemCaseSensitive(model_json, "display_name");
                            }
                            if (cJSON_IsString(display_name_item) && display_name_item->valuestring) {
                                info->display_name = dpinternal_strdup(display_name_item->valuestring);
                            }
                            
                            cJSON *version_item = cJSON_GetObjectItemCaseSensitive(model_json, "version"); 
                            if (cJSON_IsString(version_item) && version_item->valuestring) {
                                info->version = dpinternal_strdup(version_item->valuestring);
                            }

                            cJSON *description_item = cJSON_GetObjectItemCaseSensitive(model_json, "description"); 
                            if (cJSON_IsString(description_item) && description_item->valuestring) {
                                info->description = dpinternal_strdup(description_item->valuestring);
                            }
                            
                            cJSON *input_limit_item = cJSON_GetObjectItemCaseSensitive(model_json, "inputTokenLimit"); 
                            if(cJSON_IsNumber(input_limit_item)) {
                                info->input_token_limit = (long)input_limit_item->valuedouble;
                            }

                            cJSON *output_limit_item = cJSON_GetObjectItemCaseSensitive(model_json, "outputTokenLimit"); 
                             if(cJSON_IsNumber(output_limit_item)) {
                                info->output_token_limit = (long)output_limit_item->valuedouble;
                            }
                            current_model_idx++;
                        }
                    }
                } else {
                     dpinternal_safe_asprintf(&(*model_list_out)->error_message, "Expected an array for model listing response. Body: %.200s...", chunk_mem.memory ? chunk_mem.memory : "(empty)");
                    return_code = -1;
                }
                cJSON_Delete(root);
            }
        } else { 
            dpinternal_safe_asprintf(&(*model_list_out)->error_message, "list_models HTTP error %ld. Body: %.500s", (*model_list_out)->http_status_code, chunk_mem.memory ? chunk_mem.memory : "(no response body)");
            return_code = -1;
        }
    }

    free(chunk_mem.memory);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (return_code == -1 && (*model_list_out)->models == NULL && (*model_list_out)->error_message == NULL) {
         (*model_list_out)->error_message = dpinternal_strdup("Unknown error in dp_list_models before HTTP response processing.");
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