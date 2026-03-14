#include <stdio.h>
#include "test_utils.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Forward declaration of the fuzzer entry point defined in test_fuzz_response_parser_dp.c
int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size);

void run_test(const char* name, const uint8_t* data, size_t size) {
    printf("Running fuzz case: %s... ", name);
    LLVMFuzzerTestOneInput(data, size);
    printf("OK\n");
}

int main() {
    load_env_file();
    printf("Starting Fuzz Runner Sanity Check...\n");

    // Case 1: Empty input
    run_test("Empty", (const uint8_t*)"", 0);

    // Case 2: OpenAI Provider (0) + Empty JSON
    uint8_t input_openai_empty[] = {0};
    run_test("OpenAI Empty Body", input_openai_empty, sizeof(input_openai_empty));

    // Case 3: Gemini Provider (1) + Valid JSON Text
    const char* json_gemini = "{\"candidates\": [{\"content\": {\"parts\": [{\"text\": \"Hello\"}]}}]}";
    size_t len_gemini = strlen(json_gemini);
    uint8_t* input_gemini = malloc(1 + len_gemini);
    if (input_gemini) {
        input_gemini[0] = 1; // Gemini
        memcpy(input_gemini + 1, json_gemini, len_gemini);
        run_test("Gemini Valid Text", input_gemini, 1 + len_gemini);
        free(input_gemini);
    }

    // Case 4: Anthropic Provider (2) + Valid JSON Tool Call
    const char* json_anthropic = "{\"content\": [{\"type\": \"tool_use\", \"id\": \"call_1\", \"name\": \"func\", \"input\": {}}]}";
    size_t len_anthropic = strlen(json_anthropic);
    uint8_t* input_anthropic = malloc(1 + len_anthropic);
    if (input_anthropic) {
        input_anthropic[0] = 2; // Anthropic
        memcpy(input_anthropic + 1, json_anthropic, len_anthropic);
        run_test("Anthropic Tool Call", input_anthropic, 1 + len_anthropic);
        free(input_anthropic);
    }

    // Case 5: OpenAI Provider + Malformed JSON
    const char* json_malformed = "{ \"choices\": [ ... ";
    size_t len_malformed = strlen(json_malformed);
    uint8_t* input_malformed = malloc(1 + len_malformed);
    if (input_malformed) {
        input_malformed[0] = 0; // OpenAI
        memcpy(input_malformed + 1, json_malformed, len_malformed);
        run_test("OpenAI Malformed JSON", input_malformed, 1 + len_malformed);
        free(input_malformed);
    }

    // Case 6: OpenAI Provider + Tool Calls
    const char* json_openai_tools = "{\"choices\": [{\"message\": {\"tool_calls\": [{\"id\": \"call_123\", \"function\": {\"name\": \"get_weather\", \"arguments\": \"{\\\"loc\\\": \\\"NYC\\\"}\"}}]}}]}";
    size_t len_openai_tools = strlen(json_openai_tools);
    uint8_t* input_openai_tools = malloc(1 + len_openai_tools);
    if (input_openai_tools) {
        input_openai_tools[0] = 0; // OpenAI
        memcpy(input_openai_tools + 1, json_openai_tools, len_openai_tools);
        run_test("OpenAI Tool Call", input_openai_tools, 1 + len_openai_tools);
        free(input_openai_tools);
    }

    printf("Fuzz Runner Sanity Check Passed.\n");
    return 0;
}
