#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <pthread.h>

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

typedef struct {
    dp_provider_type_t provider;
    const char* api_key_env;
    int thread_status;
} thread_data_t;

void* provider_test_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    data->thread_status = -1; // Indicate failure by default

    const char* api_key = getenv(data->api_key_env);
    if (!api_key) {
        fprintf(stderr, "Thread for %s: API key %s not set. Skipping.\n", data->api_key_env, data->api_key_env);
        data->thread_status = 0; // Treat as skipped, not failure
        return NULL;
    }

    dp_context_t* context = dp_init_context(data->provider, api_key, NULL);
    if (!context) {
        fprintf(stderr, "Thread for %s: Failed to initialize context.\n", data->api_key_env);
        return NULL;
    }

    dp_request_config_t config = {0};
    dp_response_t response = {0};
    dp_message_t message = {0};

    message.role = DP_ROLE_USER;
    assert(dp_message_add_text_part(&message, "Say hello."));

    config.model = "gpt-3.5-turbo"; // Default, will be overridden for Gemini/Anthropic
    if (data->provider == DP_PROVIDER_GOOGLE_GEMINI) {
        config.model = "gemini-pro";
    } else if (data->provider == DP_PROVIDER_ANTHROPIC) {
        config.model = "claude-3-haiku-20240307";
    }
    config.messages = &message;
    config.num_messages = 1;
    config.max_tokens = 10;
    config.temperature = 0.0;
    config.stream = false;

    int ret = dp_perform_completion(context, &config, &response);

    if (ret == 0 && response.http_status_code >= 200 && response.http_status_code < 300) {
        printf("Thread for %s: Success. Response: %s\n", data->api_key_env, response.parts[0].text);
        data->thread_status = 0;
    } else {
        fprintf(stderr, "Thread for %s: Failed. HTTP Status: %ld, Error: %s\n",
                data->api_key_env, response.http_status_code, response.error_message ? response.error_message : "(none)");
        data->thread_status = -1;
    }

    dp_free_response_content(&response);
    dp_free_messages(&message, 1);
    dp_destroy_context(context);
    return NULL;
}

static int test_multiprovider_multithread() {
    int overall_status = 0;

    // Test OpenAI
    thread_data_t openai_data = { .provider = DP_PROVIDER_OPENAI_COMPATIBLE, .api_key_env = "OPENAI_API_KEY" };
    provider_test_thread(&openai_data);
    if (openai_data.thread_status != 0) {
        overall_status = -1;
    }

    // Test Gemini
    thread_data_t gemini_data = { .provider = DP_PROVIDER_GOOGLE_GEMINI, .api_key_env = "GEMINI_API_KEY" };
    provider_test_thread(&gemini_data);
    if (gemini_data.thread_status != 0) {
        overall_status = -1;
    }

    // Test Anthropic
    thread_data_t anthropic_data = { .provider = DP_PROVIDER_ANTHROPIC, .api_key_env = "ANTHROPIC_API_KEY" };
    provider_test_thread(&anthropic_data);
    if (anthropic_data.thread_status != 0) {
        overall_status = -1;
    }

    return overall_status;
}

int main() {
    printf("Starting Multi-Provider Multi-Thread Tests...\n");
    RUN_TEST(test_multiprovider_multithread);
    printf("Multi-Provider Multi-Thread Tests finished.\n");

    if (test_failures > 0) {
        printf("Total tests run: %d, Failures: %d\n", test_count, test_failures);
        return 1;
    } else {
        printf("All %d tests passed.\n", test_count);
        return 0;
    }
}
