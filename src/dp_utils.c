#include "dp_private.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char* dp_internal_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* new_s = malloc(len);
    if (!new_s) return NULL;
    memcpy(new_s, s, len);
    return new_s;
}

size_t write_memory_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    memory_struct_t* mem = (memory_struct_t*)userp;
    char* ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        fprintf(stderr, "write_memory_callback: not enough memory (realloc returned NULL)\n");
        return 0;
    }
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

