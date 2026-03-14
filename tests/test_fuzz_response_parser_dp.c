/*
 * test_fuzz_response_parser_dp.c
 * Fuzzer for dpinternal_parse_response_content
 *
 * Can be compiled with libFuzzer or as a standalone binary (define DP_FUZZ_STANDALONE).
 */

#include "disasterparty.h"
#include "dp_private.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// Clean up helper matching dp_free_response_content logic but for raw parts array
static void cleanup_parts(dp_response_part_t* parts, size_t num_parts) {
    if (!parts) return;
    for (size_t i = 0; i < num_parts; ++i) {
        free(parts[i].text);
        if (parts[i].type == DP_CONTENT_PART_TOOL_CALL) {
            free(parts[i].tool_call.id);
            free(parts[i].tool_call.function_name);
            free(parts[i].tool_call.arguments_json);
        }
    }
    free(parts);
}

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
    // Need at least one byte for provider selection
    if (Size < 1) return 0;

    // First byte determines provider
    uint8_t provider_byte = Data[0];
    dp_provider_type_t provider;
    switch (provider_byte % 3) {
        case 0: provider = DP_PROVIDER_OPENAI_COMPATIBLE; break;
        case 1: provider = DP_PROVIDER_GOOGLE_GEMINI; break;
        case 2: provider = DP_PROVIDER_ANTHROPIC; break;
        default: provider = DP_PROVIDER_OPENAI_COMPATIBLE; break;
    }

    // Rest is JSON string
    size_t json_len = Size - 1;
    char *json_str = malloc(json_len + 1);
    if (!json_str) return 0;
    memcpy(json_str, Data + 1, json_len);
    json_str[json_len] = '\0';

    dp_response_part_t* parts = NULL;
    size_t num_parts = 0;
    char* finish_reason = NULL;

    // Use a temporary context for the fuzzer
    dp_context_t* ctx = dp_init_context(provider, "fuzz-key", "https://api.example.com");
    if (!ctx) {
        free(json_str);
        return 0;
    }

    // Call the target function
    // We ignore the return value as we are testing for crashes/memory safety
    dpinternal_parse_response_content(ctx, json_str, &parts, &num_parts, &finish_reason);

    // Cleanup
    dp_destroy_context(ctx);
    cleanup_parts(parts, num_parts);
    free(finish_reason);
    free(json_str);

    return 0;
}

#ifdef DP_FUZZ_STANDALONE
int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        perror("fopen");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size < 0) {
        fclose(f);
        return 1;
    }

    uint8_t *buffer = malloc(size);
    if (!buffer) {
        fclose(f);
        return 1;
    }

    if (fread(buffer, 1, size, f) != (size_t)size) {
        free(buffer);
        fclose(f);
        return 1;
    }

    LLVMFuzzerTestOneInput(buffer, size);

    free(buffer);
    fclose(f);
    return 0;
}
#endif