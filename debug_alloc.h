#ifndef DEBUG_ALLOC_H
#define DEBUG_ALLOC_H

#include <stdio.h>
#include <stdlib.h>

static void* debug_malloc(size_t sz, const char* file, int line) {
    void* p = malloc(sz);
    printf("[ALLOC] %p = malloc(%zu) at %s:%d\n", p, sz, file, line);
    return p;
}

static void debug_free(void* p, const char* file, int line) {
    printf("[FREE ] free(%p) at %s:%d\n", p, file, line);
    free(p);
}

#define malloc(sz) debug_malloc(sz, __FILE__, __LINE__)
#define free(p)   debug_free(p, __FILE__, __LINE__)

#endif // DEBUG_ALLOC_H