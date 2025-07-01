#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <disasterparty.h>

static int test_count = 0;
static int test_failures = 0;

#define RUN_TEST(test_func) \
    do { \
        test_count++; \
        printf("Running %s...\n", #test_func); \
        if (test_func() != 0) { \
            test_failures++; \
            printf("FAIL: %s\n", #test_func); \
        } else { \
            printf("PASS: %s\n", #test_func); \
        } \
    } while (0)

static int test_free_functions_with_null() {
    // These calls should not crash or cause any errors
    dp_free_model_list(NULL);
    dp_free_response_content(NULL);
    dp_free_messages(NULL, 0);
    dp_free_file(NULL);

    // Test with a non-NULL, but empty/uninitialized message array
    dp_message_t empty_messages[1];
    memset(&empty_messages, 0, sizeof(dp_message_t));
    dp_free_messages(empty_messages, 1);

    return 0;
}

int main() {
    printf("Starting Free Functions with NULL Tests...\n");
    RUN_TEST(test_free_functions_with_null);
    printf("Free Functions with NULL Tests finished.\n");

    if (test_failures > 0) {
        printf("Total tests run: %d, Failures: %d\n", test_count, test_failures);
        return 1;
    } else {
        printf("All %d tests passed.\n", test_count);
        return 0;
    }
}
