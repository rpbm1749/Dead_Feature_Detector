# ─────────────────────────────────────────────
# nested cmake file — tests traversal
# ─────────────────────────────────────────────

# 11. add_compile_definitions in subdirectory
add_compile_definitions(FEATURE_LIB_ENABLED)        # defined here, referenced in lib

# declare a library
add_library(fixture_lib STATIC feature.cpp)

# target definitions with multiple visibility sections
target_compile_definitions(fixture_lib
    PUBLIC
        FEATURE_LIB_PUBLIC
    PRIVATE
        FEATURE_LIB_PRIVATE
    INTERFACE
        FEATURE_LIB_INTERFACE
)

# unknown target — never declared
target_compile_definitions(ghost_target PUBLIC FEATURE_GHOST)

# 13. another dead definition in nested cmake
add_definitions(-DFEATURE_LIB_DEAD) # defined, never referenced