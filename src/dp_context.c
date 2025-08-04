#define _GNU_SOURCE 
#include "disasterparty.h"
#include "dp_private.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

dp_context_t* dp_init_context(dp_provider_type_t provider, const char* api_key, const char* api_base_url) {
    return dp_init_context_with_app_info(provider, api_key, api_base_url, NULL, NULL);
}

dp_context_t* dp_init_context_with_app_info(dp_provider_type_t provider, 
                                             const char* api_key,
                                             const char* api_base_url,
                                             const char* app_name,
                                             const char* app_version) {
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
    context->api_key = dpinternal_strdup(api_key);

    if (api_base_url) {
        context->api_base_url = dpinternal_strdup(api_base_url);
    } else {
        switch (provider) {
            case DP_PROVIDER_OPENAI_COMPATIBLE:
                context->api_base_url = dpinternal_strdup(DEFAULT_OPENAI_API_BASE_URL);
                break;
            case DP_PROVIDER_GOOGLE_GEMINI:
                context->api_base_url = dpinternal_strdup(DEFAULT_GEMINI_API_BASE_URL);
                break;
            case DP_PROVIDER_ANTHROPIC:
                context->api_base_url = dpinternal_strdup(DEFAULT_ANTHROPIC_API_BASE_URL);
                break;
            default:
                context->api_base_url = NULL; 
                break;
        }
    }

    // Construct user-agent string
    if (app_name && app_version) {
        size_t user_agent_len = strlen(app_name) + strlen(app_version) + strlen(" (disasterparty/" DP_VERSION ")") + 2; // +2 for '/' and null terminator
        context->user_agent = malloc(user_agent_len);
        if (context->user_agent) {
            snprintf(context->user_agent, user_agent_len, "%s/%s (disasterparty/" DP_VERSION ")", app_name, app_version);
        }
    } else if (app_name) {
        size_t user_agent_len = strlen(app_name) + strlen(" (disasterparty/" DP_VERSION ")") + 1;
        context->user_agent = malloc(user_agent_len);
        if (context->user_agent) {
            snprintf(context->user_agent, user_agent_len, "%s (disasterparty/" DP_VERSION ")", app_name);
        }
    } else {
        context->user_agent = dpinternal_strdup("disasterparty/" DP_VERSION);
    }

    // Initialize token parameter preference (optimistically use modern parameter)
    context->token_param_preference = DP_TOKEN_PARAM_MAX_COMPLETION_TOKENS;

    if (!context->api_key || !context->api_base_url || !context->user_agent) {
        perror("Failed to allocate API key, base URL, or user-agent in Disaster Party context");
        free(context->api_key);
        free(context->api_base_url);
        free(context->user_agent);
        free(context);
        return NULL;
    }
    return context;
}

void dp_destroy_context(dp_context_t* context) {
    if (!context) return;
    free(context->api_key);
    free(context->api_base_url);
    free(context->user_agent);
    free(context);
}