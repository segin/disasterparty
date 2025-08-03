#include "disasterparty.h"
#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Helper function to extract user-agent from context (for testing purposes)
// Note: This is a test-only function that accesses internal structure
// In a real implementation, we'd need a getter function or other mechanism
extern struct dp_context_s {
    int provider;
    char* api_key;
    char* api_base_url;
    char* user_agent;
} dp_context_s;

// Test helper to get user agent string (accessing internal structure for testing)
static const char* get_user_agent_from_context(dp_context_t* context) {
    struct dp_context_s* internal_context = (struct dp_context_s*)context;
    return internal_context->user_agent;
}

int main() {
    printf("Disaster Party Library Version: %s\n", dp_get_version());
    printf("Testing user-agent functionality...\n");
    
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        fprintf(stderr, "curl_global_init() failed.\n");
        return EXIT_FAILURE;
    }
    
    int test_failures = 0;
    
    // Test 1: Default initialization (backward compatibility)
    printf("\n=== Test 1: Default initialization (backward compatibility) ===\n");
    
    dp_context_t* default_context = dp_init_context(DP_PROVIDER_ANTHROPIC, "test-key", NULL);
    if (default_context) {
        const char* user_agent = get_user_agent_from_context(default_context);
        if (user_agent && strstr(user_agent, "disasterparty/") && strstr(user_agent, DP_VERSION)) {
            printf("✓ Default context has correct user-agent: %s\n", user_agent);
        } else {
            printf("✗ Default context has incorrect user-agent: %s\n", user_agent ? user_agent : "NULL");
            test_failures++;
        }
        dp_destroy_context(default_context);
    } else {
        printf("✗ Failed to create default context\n");
        test_failures++;
    }
    
    // Test 2: Custom app info initialization
    printf("\n=== Test 2: Custom app info initialization ===\n");
    
    dp_context_t* custom_context = dp_init_context_with_app_info(
        DP_PROVIDER_ANTHROPIC, "test-key", NULL, "MyApp", "1.2.3");
    if (custom_context) {
        const char* user_agent = get_user_agent_from_context(custom_context);
        if (user_agent && 
            strstr(user_agent, "MyApp/1.2.3") && 
            strstr(user_agent, "disasterparty/") && 
            strstr(user_agent, DP_VERSION)) {
            printf("✓ Custom context has correct user-agent: %s\n", user_agent);
        } else {
            printf("✗ Custom context has incorrect user-agent: %s\n", user_agent ? user_agent : "NULL");
            test_failures++;
        }
        dp_destroy_context(custom_context);
    } else {
        printf("✗ Failed to create custom context\n");
        test_failures++;
    }
    
    // Test 3: NULL app name (should default to library name)
    printf("\n=== Test 3: NULL app name (should default to library name) ===\n");
    
    dp_context_t* null_app_context = dp_init_context_with_app_info(
        DP_PROVIDER_ANTHROPIC, "test-key", NULL, NULL, "1.0.0");
    if (null_app_context) {
        const char* user_agent = get_user_agent_from_context(null_app_context);
        if (user_agent && 
            strstr(user_agent, "disasterparty/") && 
            strstr(user_agent, DP_VERSION) &&
            !strstr(user_agent, "1.0.0")) {  // Version should not appear if app_name is NULL
            printf("✓ NULL app name context has correct user-agent: %s\n", user_agent);
        } else {
            printf("✗ NULL app name context has incorrect user-agent: %s\n", user_agent ? user_agent : "NULL");
            test_failures++;
        }
        dp_destroy_context(null_app_context);
    } else {
        printf("✗ Failed to create NULL app name context\n");
        test_failures++;
    }
    
    // Test 4: NULL app version (should still include app name)
    printf("\n=== Test 4: NULL app version (should still include app name) ===\n");
    
    dp_context_t* null_version_context = dp_init_context_with_app_info(
        DP_PROVIDER_ANTHROPIC, "test-key", NULL, "TestApp", NULL);
    if (null_version_context) {
        const char* user_agent = get_user_agent_from_context(null_version_context);
        if (user_agent && 
            strstr(user_agent, "TestApp") && 
            strstr(user_agent, "disasterparty/") && 
            strstr(user_agent, DP_VERSION)) {
            printf("✓ NULL app version context has correct user-agent: %s\n", user_agent);
        } else {
            printf("✗ NULL app version context has incorrect user-agent: %s\n", user_agent ? user_agent : "NULL");
            test_failures++;
        }
        dp_destroy_context(null_version_context);
    } else {
        printf("✗ Failed to create NULL app version context\n");
        test_failures++;
    }
    
    // Test 5: Both NULL (should be same as default)
    printf("\n=== Test 5: Both NULL (should be same as default) ===\n");
    
    dp_context_t* both_null_context = dp_init_context_with_app_info(
        DP_PROVIDER_ANTHROPIC, "test-key", NULL, NULL, NULL);
    if (both_null_context) {
        const char* user_agent = get_user_agent_from_context(both_null_context);
        if (user_agent && 
            strstr(user_agent, "disasterparty/") && 
            strstr(user_agent, DP_VERSION)) {
            printf("✓ Both NULL context has correct user-agent: %s\n", user_agent);
        } else {
            printf("✗ Both NULL context has incorrect user-agent: %s\n", user_agent ? user_agent : "NULL");
            test_failures++;
        }
        dp_destroy_context(both_null_context);
    } else {
        printf("✗ Failed to create both NULL context\n");
        test_failures++;
    }
    
    // Test 6: Empty strings (should be treated as NULL)
    printf("\n=== Test 6: Empty strings (should be treated as NULL) ===\n");
    
    dp_context_t* empty_strings_context = dp_init_context_with_app_info(
        DP_PROVIDER_ANTHROPIC, "test-key", NULL, "", "");
    if (empty_strings_context) {
        const char* user_agent = get_user_agent_from_context(empty_strings_context);
        if (user_agent && 
            strstr(user_agent, "disasterparty/") && 
            strstr(user_agent, DP_VERSION)) {
            printf("✓ Empty strings context has correct user-agent: %s\n", user_agent);
        } else {
            printf("✗ Empty strings context has incorrect user-agent: %s\n", user_agent ? user_agent : "NULL");
            test_failures++;
        }
        dp_destroy_context(empty_strings_context);
    } else {
        printf("✗ Failed to create empty strings context\n");
        test_failures++;
    }
    
    // Test 7: Memory management - multiple contexts
    printf("\n=== Test 7: Memory management - multiple contexts ===\n");
    
    dp_context_t* contexts[5];
    bool all_created = true;
    
    for (int i = 0; i < 5; i++) {
        char app_name[32], app_version[32];
        snprintf(app_name, sizeof(app_name), "TestApp%d", i);
        snprintf(app_version, sizeof(app_version), "1.0.%d", i);
        
        contexts[i] = dp_init_context_with_app_info(
            DP_PROVIDER_ANTHROPIC, "test-key", NULL, app_name, app_version);
        if (!contexts[i]) {
            all_created = false;
            break;
        }
    }
    
    if (all_created) {
        printf("✓ Successfully created multiple contexts with different user-agents\n");
        
        // Verify each has correct user-agent
        bool all_correct = true;
        for (int i = 0; i < 5; i++) {
            const char* user_agent = get_user_agent_from_context(contexts[i]);
            char expected_app[32];
            snprintf(expected_app, sizeof(expected_app), "TestApp%d/1.0.%d", i, i);
            
            if (!user_agent || !strstr(user_agent, expected_app)) {
                all_correct = false;
                printf("✗ Context %d has incorrect user-agent: %s\n", i, user_agent ? user_agent : "NULL");
                test_failures++;
                break;
            }
        }
        
        if (all_correct) {
            printf("✓ All contexts have correct unique user-agents\n");
        }
        
        // Clean up
        for (int i = 0; i < 5; i++) {
            dp_destroy_context(contexts[i]);
        }
        printf("✓ All contexts cleaned up successfully\n");
    } else {
        printf("✗ Failed to create multiple contexts\n");
        test_failures++;
        
        // Clean up any that were created
        for (int i = 0; i < 5; i++) {
            if (contexts[i]) {
                dp_destroy_context(contexts[i]);
            }
        }
    }
    
    // Test 8: Error conditions
    printf("\n=== Test 8: Error conditions ===\n");
    
    // Test with NULL API key (should fail)
    dp_context_t* null_key_context = dp_init_context_with_app_info(
        DP_PROVIDER_ANTHROPIC, NULL, NULL, "TestApp", "1.0.0");
    if (!null_key_context) {
        printf("✓ Correctly rejected NULL API key\n");
    } else {
        printf("✗ Should have rejected NULL API key\n");
        test_failures++;
        dp_destroy_context(null_key_context);
    }
    
    // Test with invalid provider (should fail)
    dp_context_t* invalid_provider_context = dp_init_context_with_app_info(
        (dp_provider_type_t)999, "test-key", NULL, "TestApp", "1.0.0");
    if (!invalid_provider_context) {
        printf("✓ Correctly rejected invalid provider\n");
    } else {
        printf("✗ Should have rejected invalid provider\n");
        test_failures++;
        dp_destroy_context(invalid_provider_context);
    }
    
    curl_global_cleanup();
    
    // Final results
    printf("\n=== Test Results ===\n");
    if (test_failures == 0) {
        printf("✓ All user-agent tests passed!\n");
        printf("User-agent test (Disaster Party) finished successfully.\n");
        return EXIT_SUCCESS;
    } else {
        printf("✗ %d test(s) failed\n", test_failures);
        printf("User-agent test (Disaster Party) finished with failures.\n");
        return EXIT_FAILURE;
    }
}