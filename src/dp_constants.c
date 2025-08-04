#include "disasterparty.h"
#include "dp_private.h"

// Default base URLs for different providers
const char* DEFAULT_OPENAI_API_BASE_URL = "https://api.openai.com/v1";
const char* DEFAULT_GEMINI_API_BASE_URL = "https://generativelanguage.googleapis.com/v1beta";
const char* DEFAULT_ANTHROPIC_API_BASE_URL = "https://api.anthropic.com/v1";

// User agent string for HTTP requests
const char* DISASTERPARTY_USER_AGENT = "disasterparty/" DP_VERSION;

/**
 * @brief Get the library version string.
 * 
 * @return const char* The version string defined in DP_VERSION.
 */
const char* dp_get_version(void) {
    return DP_VERSION;
}