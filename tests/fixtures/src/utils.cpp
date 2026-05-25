// ─────────────────────────────────────────────
// 1. FEATURE_AUDIO referenced — should resolve
//    from ${FEATURE_FLAG} → FEATURE_AUDIO chain
// ─────────────────────────────────────────────
#ifdef FEATURE_AUDIO
    void audio_init() { }
#endif

// ─────────────────────────────────────────────
// 2. FEATURE_LIST_A referenced
// ─────────────────────────────────────────────
#ifdef FEATURE_LIST_A
    void list_feature_a() { }
#endif

// ─────────────────────────────────────────────
// 3. FEATURE_LIST_C referenced — but was REMOVED
//    from the list via REMOVE_ITEM
//    expected: ReferencedNotDefined
// ─────────────────────────────────────────────
#ifdef FEATURE_LIST_C
    void list_feature_c() { }
#endif

// ─────────────────────────────────────────────
// 4. FEATURE_LIB_ENABLED referenced
//    defined in lib/CMakeLists.txt
// ─────────────────────────────────────────────
#ifdef FEATURE_LIB_ENABLED
    void lib_feature() { }
#endif