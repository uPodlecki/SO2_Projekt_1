#include "heap.h"
#include "custom_unistd.h"
#include "tested_declarations.h"
#include "rdebug.h"

int heap_setup(void) {
    // Tutaj inicjalizacja sterty, np. przydzielenie początkowego bloku pamięci
    void* initial_block = custom_sbrk(0); // Pobranie aktualnego wskaźnika sterty
    if (initial_block == (void*) -1) {
        return -1; // Nie udało się zainicjalizować sterty
    }
    // Dodatkowe ustawienia, jeśli są potrzebne
    return 0;
}

void heap_clean(void) {
    // Resetowanie sterty do stanu początkowego
    custom_sbrk(-custom_sbrk(0)); // Zwrócenie przydzielonej pamięci
}

// Pozostałe funkcje zostaną zaimplementowane później
