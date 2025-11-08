#include "heap.h"
#include "custom_unistd.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// --- Zmienne globalne ---
void*  g_heap_start = NULL;
size_t g_heap_size = 0;
bool   g_heap_initialized = false;
struct memory_block_t* g_heap_first_block = NULL;

// --- Implementacja funkcji pomocniczych ---

int calculate_checksum(const struct memory_block_t* block) {
    if (!block) return 0;
    return (int)((uintptr_t)block->prev ^ (uintptr_t)block->next ^ block->size ^ block->is_free);
}

// --- Implementacja API ---

int heap_setup(void) {
    if (g_heap_initialized) {
        heap_clean();
    }
    void* start_brk = custom_sbrk(0);
    if (start_brk == (void*)-1) return -1;

    g_heap_start = start_brk;
    g_heap_size = 0;
    g_heap_first_block = NULL;
    g_heap_initialized = true;
    return 0;
}

void heap_clean(void) {
    if (!g_heap_initialized) return;

    if (g_heap_size > 0) {
        custom_sbrk(-g_heap_size);
    }

    g_heap_start = NULL;
    g_heap_size = 0;
    g_heap_first_block = NULL;
    g_heap_initialized = false;
}

void* heap_malloc(size_t size) {
    if (!g_heap_initialized || size == 0) return NULL;
    if (heap_validate() != 0) return NULL;

    struct memory_block_t *current = g_heap_first_block;
    while (current) {
        if (current->is_free && current->size >= size) {

            // Poprawne dzielenie bloku
            size_t splittable_size = sizeof(struct memory_block_t) + 2 * FENCE_SIZE;
            if (current->size > size + splittable_size) {

                struct memory_block_t *new_free_block = (struct memory_block_t *) ((char *) current + splittable_size +
                                                                                   size);
                new_free_block->is_free = true;
                new_free_block->size = current->size - size - splittable_size;

                new_free_block->prev = current;
                new_free_block->next = current->next;
                if (current->next) {
                    current->next->prev = new_free_block;
                }
                current->next = new_free_block;
                current->size = size;

                new_free_block->checksum = calculate_checksum(new_free_block);
                if (new_free_block->next) {
                    new_free_block->next->checksum = calculate_checksum(new_free_block->next);
                }
            }

            current->is_free = false;
            current->checksum = calculate_checksum(current);

            char *user_ptr = (char *) current + sizeof(struct memory_block_t) + FENCE_SIZE;
            memset(user_ptr - FENCE_SIZE, FENCE_PATTERN, FENCE_SIZE);
            memset(user_ptr + current->size, FENCE_PATTERN, FENCE_SIZE);
            return user_ptr;
        }
        current = current->next;
    }

// Alokacja nowego bloku (bez zmian)
    const size_t total_size_needed = sizeof(struct memory_block_t) + 2 * FENCE_SIZE + size;
    void *new_memory = custom_sbrk(total_size_needed);
    if (new_memory == (void *) -1) return NULL;
    g_heap_size += total_size_needed;

    struct memory_block_t *new_block = (struct memory_block_t *) new_memory;
    new_block->size = size;
    new_block->is_free = false;
    new_block->next = NULL;

    if (g_heap_first_block == NULL) {
        new_block->prev = NULL;
        g_heap_first_block = new_block;
    } else {
        struct memory_block_t *last_block = g_heap_first_block;
        while (last_block->next != NULL) last_block = last_block->next;
        new_block->prev = last_block;
        last_block->next = new_block;
        last_block->checksum = calculate_checksum(last_block);
    }

    new_block->checksum = calculate_checksum(new_block);

    char *user_ptr = (char *) new_block + sizeof(struct memory_block_t) + FENCE_SIZE;
    memset(user_ptr - FENCE_SIZE, FENCE_PATTERN, FENCE_SIZE);
    memset(user_ptr + size, FENCE_PATTERN, FENCE_SIZE);

    return user_ptr;
}

void* heap_calloc(size_t number, size_t size) {
    if (!g_heap_initialized || number == 0 || size == 0) return NULL;

    size_t total_size = number * size;
    if (size != 0 && total_size / size != number) return NULL;

    void* ptr = heap_malloc(total_size);
    if (ptr == NULL) return NULL;

    memset(ptr, 0, total_size);
    return ptr;
}

void* heap_realloc(void* memblock, size_t size) {
    if (!g_heap_initialized) return NULL;
    // TODO: Implementacja
    (void)memblock; (void)size;
    return NULL;
}

void heap_free(void* memblock) {
    if (!g_heap_initialized || memblock == NULL) return;
    if (heap_validate() != 0) return;
    if (get_pointer_type(memblock) != pointer_valid) return;

    struct memory_block_t* block_to_free = (struct memory_block_t*)((char*)memblock - FENCE_SIZE - sizeof(struct memory_block_t));
    block_to_free->is_free = true;

    // Poprawne łączenie bloków
    size_t block_overhead = sizeof(struct memory_block_t) + 2 * FENCE_SIZE;

    if (block_to_free->next && block_to_free->next->is_free) {
        block_to_free->size += block_to_free->next->size + block_overhead;
        block_to_free->next = block_to_free->next->next;
        if (block_to_free->next) {
            block_to_free->next->prev = block_to_free;
        }
    }

    if (block_to_free->prev && block_to_free->prev->is_free) {
        block_to_free->prev->size += block_to_free->size + block_overhead;
        block_to_free->prev->next = block_to_free->next;
        if (block_to_free->next) {
            block_to_free->next->prev = block_to_free->prev;
        }
        block_to_free = block_to_free->prev;
    }

    block_to_free->checksum = calculate_checksum(block_to_free);
    if(block_to_free->next) {
        block_to_free->next->checksum = calculate_checksum(block_to_free->next);
    }
}

size_t heap_get_largest_used_block_size(void) {
    if (!g_heap_initialized || heap_validate() != 0) {
        return 0;
    }

    size_t max_size = 0;
    struct memory_block_t* current = g_heap_first_block;
    while (current) {
        if (!current->is_free) {
            if (current->size > max_size) {
                max_size = current->size;
            }
        }
        current = current->next;
    }
    return max_size;
}

enum pointer_type_t get_pointer_type(const void* const pointer) {
    if (pointer == NULL) return pointer_null;
    if (!g_heap_initialized) return pointer_unallocated;
    if (heap_validate() != 0) return pointer_heap_corrupted;

    struct memory_block_t* current = g_heap_first_block;
    while (current) {
        if ((char*)pointer >= (char*)current && (char*)pointer < ((char*)current + sizeof(struct memory_block_t))) {
            return pointer_control_block;
        }

        char* user_ptr_start = (char*)current + sizeof(struct memory_block_t) + FENCE_SIZE;
        if (!current->is_free) {
            if (pointer == user_ptr_start) return pointer_valid;
            if ((char*)pointer > user_ptr_start && (char*)pointer < (user_ptr_start + current->size)) return pointer_inside_data_block;
            if ((char*)pointer >= (user_ptr_start - FENCE_SIZE) && (char*)pointer < user_ptr_start) return pointer_inside_fences;
            if ((char*)pointer >= (user_ptr_start + current->size) && (char*)pointer < (user_ptr_start + current->size + FENCE_SIZE)) return pointer_inside_fences;
        }

        current = current->next;
    }

    return pointer_unallocated;
}

int heap_validate(void) {
    if (!g_heap_initialized) return 2;

    struct memory_block_t* current = g_heap_first_block;
    while (current) {
        if (current->checksum != calculate_checksum(current)) {
            return 3;
        }

        if (!current->is_free) {
            char* user_ptr = (char*)current + sizeof(struct memory_block_t) + FENCE_SIZE;
            for (size_t i = 0; i < FENCE_SIZE; ++i) {
                if (*((unsigned char*)user_ptr - FENCE_SIZE + i) != FENCE_PATTERN) return 1;
            }
            for (size_t i = 0; i < FENCE_SIZE; ++i) {
                if (*((unsigned char*)user_ptr + current->size + i) != FENCE_PATTERN) return 1;
            }
        }
        current = current->next;
    }
    return 0;
}
