//debug_alloc.h
#ifndef DEBUG_ALLOC_H
#define DEBUG_ALLOC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "colors.h"

typedef struct MemoryBlock {
    void* address;
    size_t size;
    int freed;
    struct MemoryBlock* next;
} MemoryBlock;

static MemoryBlock* allocated_memory = NULL;

static void* debug_malloc(size_t sz, const char* file, int line) {
    void* p = malloc(sz);
    if (p) {
        MemoryBlock* new_block = (MemoryBlock*)malloc(sizeof(MemoryBlock));
        new_block->address = p;
        new_block->size = sz;
        new_block->freed = 0;
        new_block->next = allocated_memory;
        allocated_memory = new_block;
    }
    printf("[ALLOC] %p = malloc(%zu) at %s:%d\n", p, sz, file, line);
    return p;
}

static void* debug_realloc(void* ptr, size_t sz, const char* file, int line) {
    void* new_ptr = realloc(ptr, sz);
    if (!new_ptr) {
        printf("[REALLOC] FAILED realloc(%p, %zu) at %s:%d\n", ptr, sz, file, line);
        return NULL;
    }

    MemoryBlock* current = allocated_memory;
    while (current) {
        if (current->address == ptr) {
            current->freed = 1; // old block is considered freed
            break;
        }
        current = current->next;
    }

    // Add a new memory block for the new pointer
    MemoryBlock* new_block = (MemoryBlock*)malloc(sizeof(MemoryBlock));
    new_block->address = new_ptr;
    new_block->size = sz;
    new_block->freed = 0;
    new_block->next = allocated_memory;
    allocated_memory = new_block;

    printf("[REALLOC] %p -> %p (%zu) at %s:%d\n", ptr, new_ptr, sz, file, line);
    return new_ptr;
}

static void debug_free(void* p, const char* file, int line) {
    MemoryBlock* current = allocated_memory;
    while (current) {
        if (current->address == p) {
            current->freed = 1;
            break;
        }
        current = current->next;
    }
    printf("[FREE ] free(%p) at %s:%d\n", p, file, line);
    free(p);
    p = NULL;
}

static void print_allocations() {
    printf("\n[MEMORY STATUS]\n");
    MemoryBlock* current = allocated_memory;
    while (current) {
        if (current->freed) {
            // Freed memory in green
            printf(GRN "[FREED] %p = %zu bytes\n" RESET, current->address, current->size);
        } else {
            // Not freed memory in red
            printf(RED "[ALLOCD] %p = %zu bytes\n" RESET, current->address, current->size);
        }
        current = current->next;
    }
}

#define malloc(sz) debug_malloc(sz, __FILE__, __LINE__)
#define realloc(p,sz) debug_realloc(p, sz, __FILE__, __LINE__)
#define free(p)   debug_free(p, __FILE__, __LINE__)

#endif