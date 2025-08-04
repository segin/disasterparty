#include "dp_private.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// String duplication utility
char* dpinternal_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* new_s = malloc(len);
    if (!new_s) return NULL;
    memcpy(new_s, s, len);
    return new_s;
}

// Helper function to safely use asprintf and handle allocation failures
int dpinternal_safe_asprintf(char** strp, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int result = vasprintf(strp, fmt, args);
    va_end(args);
    if (result == -1) {
        *strp = NULL;
    }
    return result;
}



// Token counting function
int dp_count_tokens(dp_context_t* context,
                    const dp_request_config_t* request_config,
                    size_t* token_count_out) {
    if (!context || !request_config || !token_count_out) {
        return -1;
    }
    *token_count_out = 0;

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
        case DP_PROVIDER_OPENAI_COMPATIBLE:
            fprintf(stderr, "dp_count_tokens is not supported for the OpenAI provider.\n");
            return_code = -1;
            goto cleanup;
        case DP_PROVIDER_GOOGLE_GEMINI:
            json_payload_str = dpinternal_build_gemini_count_tokens_json_payload_with_cjson(request_config);
            if (!json_payload_str) {
                fprintf(stderr, "Failed to build JSON payload for dp_count_tokens (Gemini).\n");
                goto cleanup;
            }
            snprintf(url, sizeof(url), "%s/models/%s:countTokens?key=%s",
                     context->api_base_url, request_config->model, context->api_key);
            break;
        case DP_PROVIDER_ANTHROPIC:
            json_payload_str = dpinternal_build_anthropic_count_tokens_json_payload_with_cjson(request_config);
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
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, dpinternal_write_memory_callback);
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