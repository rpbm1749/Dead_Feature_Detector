#include "../include/config.h"
#include <stdio.h>

// ─────────────────────────────────────────────
// 1. simple #ifdef usage
// ─────────────────────────────────────────────
#ifdef FEATURE_LOGGING
    void log_init() { printf("logging enabled\n"); }
#endif

// ─────────────────────────────────────────────
// 2. nested #ifdef blocks
// ─────────────────────────────────────────────
#ifdef FEATURE_MULTILINE_X
    #ifdef FEATURE_MULTILINE_Y
        void use_xy() { }
    #endif
#endif

// ─────────────────────────────────────────────
// 3. FEATURE_MULTI_A referenced (FEATURE_MULTI_B not referenced here)
// ─────────────────────────────────────────────
#ifdef FEATURE_MULTI_A
    void multi_a() { }
#endif

// ─────────────────────────────────────────────
// 4. ENABLE_TESTS referenced
// ─────────────────────────────────────────────
#ifdef ENABLE_TESTS
    void run_tests() { }
#endif

// ─────────────────────────────────────────────
// 5. platform macro — referenced but not in CMake
//    expected: ReferencedNotDefined (known false positive category)
// ─────────────────────────────────────────────
#ifdef _WIN32
    void win_init() { }
#endif

// ─────────────────────────────────────────────
// 6. VERSION referenced
// ─────────────────────────────────────────────
#ifdef VERSION
    void print_version() { printf("version defined\n"); }
#endif

int main() {
    return 0;
}