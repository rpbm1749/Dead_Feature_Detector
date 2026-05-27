#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

// A macro referenced in a source file via
// #ifdef, #ifndef, #if defined(), #elif defined()
struct SourceMacroRef {
    std::string macro_name;
    std::string file_path;
    int line;
    int span_lines;         // how many lines the ifdef block covers
    bool is_negated;
};

// A macro defined via add_definitions(-DFOO or -DFOO=1)
struct CMakeDefinition {
    std::string macro_name;     // stripped of -D prefix
    std::optional<std::string> value;   // present if -DFOO=VAL
    std::string file_path;
    int line;
    bool needs_resolution = false;  // whether the value contains unresolved variables
};

// An option() declaration
// option(FOO "description" ON/OFF)
struct CMakeOption {
    std::string name;
    std::string description;
    std::string default_value;  // usually ON or OFF
    std::string file_path;
    int line;
};

// A macro from target_compile_definitions(target PUBLIC/PRIVATE -DFOO)
struct CMakeTargetDefinition {
    std::string macro_name;
    std::optional<std::string> value;
    std::string target_name;
    std::string visibility;     // PUBLIC, PRIVATE, INTERFACE
    std::string file_path;
    int line;
    bool needs_resolution = false;  // whether the value contains unresolved variables
};

struct VariableValue {
    std::string raw_value;
    std::string file_path;
    int line;
};

struct CMakeVariable {
    std::string name;
    std::vector<VariableValue> possible_values;
    std::string file_path;   // first seen
    int line;                // first seen
    bool is_option = false;    // whether this variable was declared as an option()
    bool is_cache = false;     // whether this variable was set with CACHE
    std::optional<std::string> cache_type;      // BOOL, STRING, PATH, FILEPATH
    std::optional<std::string> default_value;   // for option() defaults
};

enum class TargetType {
    Executable,
    Library,          // STATIC, SHARED, MODULE — all treated the same
    InterfaceLibrary, // INTERFACE — propagates to consumers
    Unknown           // target used in target_compile_definitions but never declared
};

struct CMakeTarget {
    std::string name;
    TargetType type;
    std::string file_path;
    int line;
};

struct UnknownTargetRef {
    std::string target_name;
    std::string file_path;
    int line;
};

// ─────────────────────────────────────────────
//  RESOLVER SIDE
// ─────────────────────────────────────────────

// Represents one fully expanded possible value of a variable
// e.g. Y = ${X}_ON where X = {A, B} → expanded = {A_ON, B_ON}
struct ExpandedValue {
    std::string value;
    bool is_fully_resolved;     // false if any variable in chain was unresolvable
};

// Tracks unresolved variable references
// e.g. add_definitions(${UNKNOWN_VAR}) — UNKNOWN_VAR never declared
struct UnresolvedRef {
    std::string expression;         // the raw unresolved expression
    std::string referencing_var;    // which variable/definition used it
    std::string file_path;
    int line;

    enum class Context {
        AddDefinitions,
        TargetDefinitions,
        VariableExpansion
    } context;
};

// ─────────────────────────────────────────────
//  ANALYSIS SIDE
// ─────────────────────────────────────────────

enum class MacroStatus {
    DefinedAndReferenced,       // healthy
    DefinedNotReferenced,       // potentially dead
    ReferencedNotDefined,       // potentially missing / external
    Indeterminate               // involves unresolvable variables
};

struct MacroLocation {
    std::string file_path;
    int line;
    std::optional<std::string> target_name;  // only for target definitions
};

struct MacroReport {
    std::string macro_name;
    MacroStatus status;
    std::vector<MacroLocation> defined_at;      // all definition sites
    std::vector<MacroLocation> referenced_at;   // all reference sites
    float confidence;
};

// ─────────────────────────────────────────────
//  RESOLVER OUTPUT
// ─────────────────────────────────────────────

struct ResolvedData {
    std::unordered_map<std::string, std::vector<MacroLocation>> defined_macros;      // macro_name -> all definition sites
    std::unordered_map<std::string, std::vector<MacroLocation>> referenced_macros;   // macro_name -> all reference sites
    std::vector<UnresolvedRef> unresolved;  // unresolvable variable references
};

// ─────────────────────────────────────────────
//  TOP-LEVEL COLLECTED DATA
//  This is what gets passed between pipeline stages
// ─────────────────────────────────────────────

struct ProjectData {
    // raw parsed data
    std::vector<SourceMacroRef>         source_refs;
    std::vector<CMakeDefinition>        cmake_definitions;
    std::vector<CMakeOption>            cmake_options;
    std::vector<CMakeTargetDefinition>  target_definitions;
    std::vector<CMakeVariable>          cmake_variables;
    std::unordered_map<std::string, CMakeVariable> variable_map;  // variable name -> CMakeVariable
    std::unordered_map<std::string, CMakeTarget> target_map;  //Target files
    std::vector<UnknownTargetRef>       unknown_target_refs;

    // resolver output
    std::vector<UnresolvedRef>          unresolved_refs;

    // final analysis output
    std::vector<MacroReport>            report;
};