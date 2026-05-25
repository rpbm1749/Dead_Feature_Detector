#pragma once

// ─────────────────────────────────────────────
// 1. simple #ifdef — healthy (defined in CMake)
// ─────────────────────────────────────────────
#ifdef FEATURE_LOGGING
    #define LOG(x) printf(x)
#else
    #define LOG(x)
#endif

// ─────────────────────────────────────────────
// 2. simple #ifndef
// ─────────────────────────────────────────────
#ifndef FEATURE_LOGGING
    #define LOG(x)      // block is dead since FEATURE_LOGGING is defined
#endif

// ─────────────────────────────────────────────
// 3. #if defined() single macro
// ─────────────────────────────────────────────
#if defined(FEATURE_MULTILINE_X)
    #define USE_X 1
#endif

// ─────────────────────────────────────────────
// 4. #if defined() && defined() — multiple macros
// ─────────────────────────────────────────────
#if defined(FEATURE_MULTILINE_X) && defined(FEATURE_MULTILINE_Y)
    #define USE_XY 1
#endif

// ─────────────────────────────────────────────
// 5. #if !defined() — negated
// ─────────────────────────────────────────────
#if !defined(FEATURE_DEAD_GLOBAL)
    #define FALLBACK 1  // block is dead since FEATURE_DEAD_GLOBAL is defined
#endif

// ─────────────────────────────────────────────
// 6. referenced but never defined in CMake
// ─────────────────────────────────────────────
#ifdef FEATURE_NEVER_DEFINED
    #define GHOST 1     // macro referenced but not in any CMakeLists.txt
#endif

// ─────────────────────────────────────────────
// 7. #elif defined()
// ─────────────────────────────────────────────
#if defined(FEATURE_MULTI_A)
    #define MODE_A 1
#elif defined(FEATURE_MULTI_B)
    #define MODE_B 1    // FEATURE_MULTI_B defined but only referenced here via elif
#endif