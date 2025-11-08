#
# DANTE :: System Automatycznych Testów Jednostkowych oraz Akceptacyjnych
# Tomasz Jaworski, 2018-2020
#
# Plik wygenerowany automatycznie
# Znacznik czasowy: 2024-11-10 13:45:49.281268
#

cmake_minimum_required(VERSION 3.13)
project(project1 C)

# Przyjmij standard C11
set(CMAKE_C_STANDARD 11)
# Ustaw opcje kompilatora (z raportu Dante)
add_compile_options(
      "-Wno-parentheses"
      "-Wall"
      "-Wextra"
      "-std=c11"
      "-fmax-errors=5"
      "-xc"
      "-Wno-error=implicit-fallthrough"
      "-D_GNU_SOURCE"
      "-Werror"
      "-Wno-error=parentheses"
      "-Werror=vla"
      "-D_ANSI_OUTPUT"
      "-ggdb3"
      "-Wno-error=unused-parameter"
      "-DINSIDE_DANTE"
      "-fdiagnostics-color"
      "-pedantic"

)

# Ustaw opcje konsolidatora (z raportu Dante)
add_link_options(
)

# Utwórz target 'project1' i dodaj do niego pliki źródlowe:
add_executable(project1
        "heap.c"
        "main.c"
        "unit_helper_v2.c"
        "unit_test_v2.c"
        "rdebug.c"
        "memmanager.c"
)

# Dołącz niezbędne biblioteki
target_link_libraries(project1 PRIVATE pthread "-Wl,--wrap=main")