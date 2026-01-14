#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdbool.h>

// --- Stałe i definicje ---
#define FENCE_SIZE 4
#define FENCE_PATTERN 0xEF

// --- Typy danych ---

// Typ wyliczeniowy dla klasyfikacji wskaźników
enum pointer_type_t
{
    pointer_null,
    pointer_heap_corrupted,
    pointer_control_block,
    pointer_inside_fences,
    pointer_inside_data_block,
    pointer_unallocated,
    pointer_valid
};

// Struktura kontrolna bloku pamięci
struct memory_block_t {
    struct memory_block_t* prev; // Wskaźnik na poprzedni blok
    struct memory_block_t* next; // Wskaźnik na następny blok
    size_t size;                 // Rozmiar bloku danych użytkownika
    bool is_free;                // Czy blok jest wolny?

    // Suma kontrolna do walidacji nagłówka
    int checksum;
};


// --- Prototypy funkcji ---

int heap_setup(void);
void heap_clean(void);

void* heap_malloc(size_t size);
void* heap_calloc(size_t number, size_t size);
void* heap_realloc(void* memblock, size_t size);
void heap_free(void* memblock);

size_t heap_get_largest_used_block_size(void);
enum pointer_type_t get_pointer_type(const void* const pointer);
int heap_validate(void);

// Funkcja pomocnicza do obliczania sumy kontrolnej
int calculate_checksum(const struct memory_block_t* block);


#endif /* HEAP_H */
