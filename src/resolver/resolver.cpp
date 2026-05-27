#include "resolver.hpp"
#include <iostream>
#include <regex>
#include <unordered_set>

// ─────────────────────────────────────────────
//  Variable expansion
// ─────────────────────────────────────────────

// Recursively expands ${VAR} references in a string
// Returns all possible fully/partially expanded values
// depth guard prevents infinite loops from circular references
static std::vector<ExpandedValue> expand(
    const std::string& raw,
    const std::unordered_map<std::string, CMakeVariable>& variable_map,
    int depth = 0
) {
    // guard against circular references
    if (depth > 10) {
        ExpandedValue v;
        v.value             = raw;
        v.is_fully_resolved = false;
        return { v };
    }

    // find first ${VAR} reference
    static const std::regex var_ref(R"(\$\{([a-zA-Z_][a-zA-Z0-9_]*)\})");
    std::smatch match;

    if (!std::regex_search(raw, match, var_ref)) {
        // no variable references — already fully resolved
        ExpandedValue v;
        v.value             = raw;
        v.is_fully_resolved = true;
        return { v };
    }

    std::string var_name  = match[1].str();
    std::string prefix    = match.prefix().str();
    std::string suffix    = match.suffix().str();

    // look up variable in map
    auto it = variable_map.find(var_name);
    if (it == variable_map.end()) {
        // variable not declared anywhere — unresolvable
        ExpandedValue v;
        v.value             = raw;
        v.is_fully_resolved = false;
        return { v };
    }

    const CMakeVariable& var = it->second;
    if (var.possible_values.empty()) {
        ExpandedValue v;
        v.value             = raw;
        v.is_fully_resolved = false;
        return { v };
    }

    // for each possible value of this variable,
    // substitute and recurse on the result
    std::vector<ExpandedValue> results;

    for (const auto& possible_val : var.possible_values) {
        std::string substituted = prefix + possible_val.raw_value + suffix;

        // recurse to expand any remaining ${} references
        auto sub_results = expand(substituted, variable_map, depth + 1);
        for (auto& r : sub_results) {
            // if the substituted value itself contained unresolved refs,
            // propagate is_fully_resolved = false
            if (possible_val.raw_value.find("${") != std::string::npos) {
                r.is_fully_resolved = false;
            }
            results.push_back(r);
        }
    }

    return results;
}

// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────

static MacroLocation make_location(
    const std::string& file_path,
    int line,
    const std::optional<std::string>& target_name = std::nullopt
) {
    MacroLocation loc;
    loc.file_path   = file_path;
    loc.line        = line;
    loc.target_name = target_name;
    return loc;
}

static void insert_defined(
    ResolvedData& resolved,
    const std::string& macro_name,
    const MacroLocation& loc
) {
    resolved.defined_macros[macro_name].push_back(loc);
}

static void insert_referenced(
    ResolvedData& resolved,
    const std::string& macro_name,
    const MacroLocation& loc
) {
    resolved.referenced_macros[macro_name].push_back(loc);
}

// ─────────────────────────────────────────────
//  Main entry point
// ─────────────────────────────────────────────

ResolvedData resolve(const ProjectData& data) {
    ResolvedData resolved;

    // ── 1. cmake_definitions ─────────────────
    for (const auto& def : data.cmake_definitions) {
        if (!def.needs_resolution) {
            // clean macro name — insert directly
            insert_defined(
                resolved,
                def.macro_name,
                make_location(def.file_path, def.line)
            );
            continue;
        }

        // needs expansion
        auto expanded = expand(def.macro_name, data.variable_map);

        for (const auto& e : expanded) {
            if (e.is_fully_resolved) {
                insert_defined(
                    resolved,
                    e.value,
                    make_location(def.file_path, def.line)
                );
            } else {
                // flag as unresolved
                UnresolvedRef ref;
                ref.expression      = def.macro_name;
                ref.referencing_var = e.value;
                ref.file_path       = def.file_path;
                ref.line            = def.line;
                ref.context         = UnresolvedRef::Context::AddDefinitions;
                resolved.unresolved.push_back(ref);
            }
        }
    }

    // ── 2. target_definitions ────────────────
    for (const auto& def : data.target_definitions) {
        if (!def.needs_resolution) {
            insert_defined(
                resolved,
                def.macro_name,
                make_location(def.file_path, def.line, def.target_name)
            );
            continue;
        }

        auto expanded = expand(def.macro_name, data.variable_map);

        for (const auto& e : expanded) {
            if (e.is_fully_resolved) {
                insert_defined(
                    resolved,
                    e.value,
                    make_location(def.file_path, def.line, def.target_name)
                );
            } else {
                UnresolvedRef ref;
                ref.expression      = def.macro_name;
                ref.referencing_var = e.value;
                ref.file_path       = def.file_path;
                ref.line            = def.line;
                ref.context         = UnresolvedRef::Context::TargetDefinitions;
                resolved.unresolved.push_back(ref);
            }
        }
    }

    // ── 3. source_refs ───────────────────────
    // no expansion needed — macro names in source are always literal
    for (const auto& ref : data.source_refs) {
        insert_referenced(
            resolved,
            ref.macro_name,
            make_location(ref.file_path, ref.line)
        );
    }

    return resolved;
}