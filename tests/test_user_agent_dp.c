#include "disasterparty.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    dp_context_t* context = dp_init_context(DP_PROVIDER_OPENAI_COMPATIBLE, "dummy_key", NULL);
    if (!context) {
        fprintf(stderr, "Failed to initialize Disaster Party context for OpenAI.\n");
        return EXIT_FAILURE;
    }

    // Test default user agent
    if (strcmp(dp_get_user_agent(context), "disasterparty/0.5.0") != 0) {
        fprintf(stderr, "FAIL: Default user agent is incorrect. Expected: disasterparty/0.5.0, Got: %s\n", dp_get_user_agent(context));
        return EXIT_FAILURE;
    }

    // Test setting user agent
    dp_set_user_agent(context, "my_app/1.2.3 (disasterparty/0.5.0)");
    if (strcmp(dp_get_user_agent(context), "my_app/1.2.3 (disasterparty/0.5.0)") != 0) {
        fprintf(stderr, "FAIL: Custom user agent is incorrect. Expected: my_app/1.2.3 (disasterparty/0.5.0), Got: %s\n", dp_get_user_agent(context));
        return EXIT_FAILURE;
    }

    dp_destroy_context(context);

    printf("PASS: User agent tests passed.\n");
    return EXIT_SUCCESS;
}
