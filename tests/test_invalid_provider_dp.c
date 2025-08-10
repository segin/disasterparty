#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Testing dp_init_context with an invalid provider enum...\n");

    // An intentionally invalid provider type
    dp_provider_type_t invalid_provider_type = (dp_provider_type_t)999;

    dp_context_t* context = dp_init_context(invalid_provider_type, "dummy_key", NULL);

    if (context == NULL) {
        printf("SUCCESS: dp_init_context correctly returned NULL for an invalid provider.\n");
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "FAILURE: dp_init_context did not return NULL for an invalid provider.\n");
        dp_destroy_context(context);
        return EXIT_FAILURE;
    }
}