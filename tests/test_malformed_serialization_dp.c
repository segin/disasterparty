#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

static int test_malformed_serialization() {
    dp_message_t* messages = NULL;
    size_t num_messages = 0;
    int ret;

    // Test with malformed JSON: missing closing bracket
    const char* malformed_json_1 = "[{\"role\":\"user\",\"parts\":[{\"type\":\"text\",\"text\":\"Hello\"}] ";
    ret = dp_deserialize_messages_from_json_str(malformed_json_1, &messages, &num_messages);
    assert(ret == -1);
    assert(messages == NULL);
    assert(num_messages == 0);

    // Test with malformed JSON: invalid role
    const char* malformed_json_2 = "[{\"role\":\"invalid\",\"parts\":[{\"type\":\"text\",\"text\":\"Hello\"}]}]";
    ret = dp_deserialize_messages_from_json_str(malformed_json_2, &messages, &num_messages);
    assert(ret == 0); // Should still parse, role defaults to user
    assert(messages != NULL);
    assert(num_messages == 1);
    assert(messages[0].role == DP_ROLE_USER);
    dp_free_messages(messages, num_messages);
    messages = NULL; num_messages = 0;

    // Test with malformed JSON: missing parts array
    const char* malformed_json_3 = "[{\"role\":\"user\"}]";
    ret = dp_deserialize_messages_from_json_str(malformed_json_3, &messages, &num_messages);
    assert(ret == 0); // Should parse, but message will have 0 parts
    assert(messages != NULL);
    assert(num_messages == 1);
    assert(messages[0].num_parts == 0);
    dp_free_messages(messages, num_messages);
    messages = NULL; num_messages = 0;

    // Test with malformed JSON: invalid part type
    const char* malformed_json_4 = "[{\"role\":\"user\",\"parts\":[{\"type\":\"invalid\",\"text\":\"Hello\"}]}]";
    ret = dp_deserialize_messages_from_json_str(malformed_json_4, &messages, &num_messages);
    assert(ret == 0); // Should parse, but part will be ignored or default
    assert(messages != NULL);
    assert(num_messages == 1);
    assert(messages[0].num_parts == 0); // Invalid part type should not be added
    dp_free_messages(messages, num_messages);
    messages = NULL; num_messages = 0;

    return 0;
}

int main() {
    printf("Starting Malformed Serialization Tests...\n");
    RUN_TEST(test_malformed_serialization);
    printf("Malformed Serialization Tests finished.\n");

    if (test_failures > 0) {
        printf("Total tests run: %d, Failures: %d\n", test_count, test_failures);
        return 1;
    } else {
        printf("All %d tests passed.\n", test_count);
        return 0;
    }
}
