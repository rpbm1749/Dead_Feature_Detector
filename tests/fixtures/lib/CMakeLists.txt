# ─────────────────────────────────────────────
# nested cmake file — tests traversal
# ─────────────────────────────────────────────

# 11. add_compile_definitions in subdirectory
add_compile_definitions(FEATURE_LIB_ENABLED)        # defined here, referenced in lib

# 12. target_compile_definitions placeholder
# (not implemented yet but good to have in fixture)
# target_compile_definitions(mylib PUBLIC FEATURE_LIB_TARGET)

# 13. another dead definition in nested cmake
add_definitions(-DFEATURE_LIB_DEAD)                 # defined, never referenced