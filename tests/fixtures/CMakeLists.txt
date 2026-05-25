cmake_minimum_required(VERSION 3.16)
project(fixture_project CXX)

# ─────────────────────────────────────────────
# 1. simple add_definitions
# ─────────────────────────────────────────────
add_definitions(-DFEATURE_LOGGING)          # defined, referenced in source → healthy
add_definitions(-DFEATURE_DEAD_GLOBAL)      # defined, never referenced → dead

# ─────────────────────────────────────────────
# 2. multiple on one line
# ─────────────────────────────────────────────
add_definitions(-DFEATURE_MULTI_A -DFEATURE_MULTI_B)   # both defined, only A referenced

# ─────────────────────────────────────────────
# 3. multi-line
# ─────────────────────────────────────────────
add_definitions(
    -DFEATURE_MULTILINE_X
    -DFEATURE_MULTILINE_Y
)

# ─────────────────────────────────────────────
# 4. with value
# ─────────────────────────────────────────────
add_definitions(-DVERSION=3)
add_compile_definitions(BUILD_TYPE=release)

# ─────────────────────────────────────────────
# 5. simple set()
# ─────────────────────────────────────────────
set(MY_PREFIX "FEATURE")
set(MY_SUFFIX "ON")

# ─────────────────────────────────────────────
# 6. variable reference — resolvable chain
# ─────────────────────────────────────────────
set(FEATURE_FLAG ${MY_PREFIX}_AUDIO)        # expands to FEATURE_AUDIO
add_definitions(-D${FEATURE_FLAG})          # should resolve to -DFEATURE_AUDIO

# ─────────────────────────────────────────────
# 7. variable reference — unresolvable
# ─────────────────────────────────────────────
add_definitions(-D${UNKNOWN_VAR}_FEATURE)   # UNKNOWN_VAR never declared → indeterminate

# ─────────────────────────────────────────────
# 8. set() with CACHE
# ─────────────────────────────────────────────
set(CACHE_VAR "some_value" CACHE STRING "a cache variable")

# ─────────────────────────────────────────────
# 9. option()
# ─────────────────────────────────────────────
option(ENABLE_TESTS "Enable test suite" ON)
option(ENABLE_BENCHMARKS "Enable benchmarks" OFF)   # defined, never referenced → dead

# ─────────────────────────────────────────────
# 10. list() operations
# ─────────────────────────────────────────────
set(FEATURE_LIST "FEATURE_LIST_A" "FEATURE_LIST_B" "FEATURE_LIST_C")
list(APPEND FEATURE_LIST "FEATURE_LIST_D")
list(PREPEND FEATURE_LIST "FEATURE_LIST_PREPEND")
list(INSERT FEATURE_LIST 1 "FEATURE_LIST_INSERT")
list(REMOVE_ITEM FEATURE_LIST "FEATURE_LIST_C")     # C removed, should not appear

# include subdirectory
add_subdirectory(lib)