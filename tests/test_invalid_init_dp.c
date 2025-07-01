#include <stdio.h>
#include <stdlib.h>
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

static int test_invalid_init() {
    dp_context_t* context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, NULL, NULL);
    assert(context == NULL);

    context = dp_init_context(DP_PROVIDER_GOOGLE_GEMINI, NULL, NULL);
    assert(context == NULL);

    context = dp_init_context(DP_PROVIDER_ANTHROPIC, NULL, NULL);
    assert(context == NULL);

    return 0;
}

int main() {
    printf("Starting Invalid Init Tests...\n");
    RUN_TEST(test_invalid_init);
    printf("Invalid Init Tests finished.\n");

    if (test_failures > 0) {
        printf("Total tests run: %d, Failures: %d\n", test_count, test_failures);
        return 1;
    } else {
        printf("All %d tests passed.\n", test_count);
        return 0;
    }
}
