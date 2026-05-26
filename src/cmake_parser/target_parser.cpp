#include "target_parser.hpp"

#include <sstream>
#include <algorithm>
#include <iostream>
#include <unordered_set>

// ─────────────────────────────────────────────
//  Shared utilities
// ─────────────────────────────────────────────

static std::vector<std::string> tokenize(const std::string& args_str) {
    std::vector<std::string> tokens;
    std::istringstream ss(args_str);
    std::string token;

    while (ss >> std::ws) {
        if (ss.peek() == EOF) break;
        if (ss >> token) {
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
    }

    return tokens;
}

static std::pair<std::string, std::optional<std::string>> strip_and_split(
    const std::string& token
) {
    std::string clean = token;

    if (clean.size() >= 2 && clean[0] == '-' && clean[1] == 'D') {
        clean = clean.substr(2);
    }

    auto eq = clean.find('=');
    if (eq != std::string::npos) {
        return { clean.substr(0, eq), clean.substr(eq + 1) };
    }

    return { clean, std::nullopt };
}

// keywords that appear in target_compile_definitions
// but are not macro definitions
static const std::unordered_set<std::string> VISIBILITY_KEYWORDS = {
    "PUBLIC", "PRIVATE", "INTERFACE"
};

// keywords that appear in add_library / add_executable
// but are not target names or types
static const std::unordered_set<std::string> IGNORED_TARGET_FLAGS = {
    "WIN32", "MACOSX_BUNDLE", "EXCLUDE_FROM_ALL"
};

// ─────────────────────────────────────────────
//  add_library()
// ─────────────────────────────────────────────

void process_add_library(
    const std::string& args,
    const std::string& file_path,
    int start_line,
    std::unordered_map<std::string, CMakeTarget>& target_map
) {
    auto tokens = tokenize(args);
    if (tokens.empty()) return;

    const std::string& name = tokens[0];
    if (name.empty()) return;

    // determine type from second token if present
    TargetType type = TargetType::Library;
    if (tokens.size() >= 2) {
        const std::string& type_token = tokens[1];
        if (type_token == "INTERFACE") {
            type = TargetType::InterfaceLibrary;
        } else if (type_token == "ALIAS") {
            // ALIAS targets point to another target
            // we skip them — they add no new definitions
            return;
        }
        // STATIC, SHARED, MODULE all collapse to Library
    }

    CMakeTarget target;
    target.name      = name;
    target.type      = type;
    target.file_path = file_path;
    target.line      = start_line;

    // last write wins — if target is redeclared, update it
    target_map[name] = target;
}

// ─────────────────────────────────────────────
//  add_executable()
// ─────────────────────────────────────────────

void process_add_executable(
    const std::string& args,
    const std::string& file_path,
    int start_line,
    std::unordered_map<std::string, CMakeTarget>& target_map
) {
    auto tokens = tokenize(args);
    if (tokens.empty()) return;

    const std::string& name = tokens[0];
    if (name.empty()) return;

    // skip ALIAS executables same as libraries
    if (tokens.size() >= 2 && tokens[1] == "ALIAS") return;

    CMakeTarget target;
    target.name      = name;
    target.type      = TargetType::Executable;
    target.file_path = file_path;
    target.line      = start_line;

    target_map[name] = target;
}

// ─────────────────────────────────────────────
//  target_compile_definitions()
// ─────────────────────────────────────────────

// Handles multiple visibility sections in one call:
//   target_compile_definitions(mylib
//       PUBLIC   FOO BAR
//       PRIVATE  BAZ
//       INTERFACE QUX
//   )

void process_target_compile_definitions(
    const std::string& args,
    const std::string& file_path,
    int start_line,
    const std::unordered_map<std::string, CMakeTarget>& target_map,
    std::vector<CMakeTargetDefinition>& target_definitions,
    std::vector<UnknownTargetRef>& unknown_target_refs
) {
    auto tokens = tokenize(args);
    if (tokens.empty()) return;

    const std::string& target_name = tokens[0];
    if (target_name.empty()) return;

    // check if target was declared
    auto target_it = target_map.find(target_name);
    if (target_it == target_map.end()) {
        // unknown target — flag it
        UnknownTargetRef ref;
        ref.target_name = target_name;
        ref.file_path   = file_path;
        ref.line        = start_line;
        unknown_target_refs.push_back(ref);
        // still parse definitions — we don't want to lose the data
        // just because the target wasn't declared in our traversal
    }

    // scan remaining tokens, tracking current visibility
    std::string current_visibility = "PUBLIC";  // default if none specified

    for (int i = 1; i < static_cast<int>(tokens.size()); ++i) {
        const std::string& token = tokens[i];

        // visibility keyword — update current section
        if (VISIBILITY_KEYWORDS.count(token)) {
            current_visibility = token;
            continue;
        }

        // skip empty tokens
        if (token.empty()) continue;

        auto [macro_name, macro_value] = strip_and_split(token);
        if (macro_name.empty()) continue;

        CMakeTargetDefinition def;
        def.macro_name       = macro_name;
        def.value            = macro_value;
        def.target_name      = target_name;
        def.visibility       = current_visibility;
        def.file_path        = file_path;
        def.line             = start_line;
        def.needs_resolution = macro_name.find("${") != std::string::npos;

        target_definitions.push_back(def);
    }
}