#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "disasterparty.h"

// Simple helper to load .env file if it exists
static inline void load_env_file() {
    FILE* fp = fopen("../.env", "r");
    if (!fp) {
        fp = fopen(".env", "r");
    }
    if (!fp) return;

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        // Trim newline
        line[strcspn(line, "\r\n")] = 0;
        
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\0') continue;
        
        char* eq = strchr(line, '=');
        if (eq) {
            *eq = '\0';
            const char* key = line;
            const char* val = eq + 1;
            
            // Only set if not already set in environment
            if (!getenv(key)) {
                setenv(key, val, 1);
            }
        }
    }
    fclose(fp);
}

// Helper to case-insensitive substring search
static inline const char* stristr(const char* haystack, const char* needle) {
    if (!haystack || !needle) return NULL;
    
    // Simple implementation for test utils
    // Not efficient but fine for short error messages
    size_t needle_len = strlen(needle);
    while (*haystack) {
        if (strncasecmp(haystack, needle, needle_len) == 0) {
            return haystack;
        }
        haystack++;
    }
    return NULL;
}

// Helper to detect if failure is due to billing/quota issues
static inline bool is_billing_or_quota_error(const dp_response_t* response) {
    if (!response) return false;

    // HTTP 429 is often Too Many Requests / Quota Exceeded
    // HTTP 402 is Payment Required (rarely used but possible)
    
    // Note: 429 can also be rate limiting, but for tests running sequentially
    // on a fresh key, it's almost always quota if it fails immediately.
    if (response->http_status_code == 429 || response->http_status_code == 402) {
        return true;
    }

    if (response->error_message) {
        // Common substrings for billing/quota errors
        const char* keywords[] = {
            "quota",
            "billing",
            "credit",
            "balance",
            "payment",
            "insufficient_quota",
            "rate limit" // Anthropic returns 429 for both rate limit and overloaded
        };
        
        for (size_t i = 0; i < sizeof(keywords)/sizeof(keywords[0]); i++) {
            if (stristr(response->error_message, keywords[i]) != NULL) {
                return true;
            }
        }
    }
    
    return false;
}

#endif // TEST_UTILS_H