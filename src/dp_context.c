#include "disasterparty.h"
#include "dp_private.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

const char* dp_get_version(void) {
    return DP_VERSION;
}

const char* dp_get_user_agent(dp_context_t* context) {
    if (!context) return NULL;
    return context->user_agent ? context->user_agent : DISASTERPARTY_USER_AGENT;
}

void dp_set_user_agent(dp_context_t* context, const char* user_agent) {
    if (!context) return;
    free(context->user_agent);
    context->user_agent = dp_internal_strdup(user_agent);
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
    context->user_agent = NULL; 

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
    free(context->user_agent);
    free(context);
}