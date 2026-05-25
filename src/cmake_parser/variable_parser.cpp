#include "variable_parser.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>
#include <iomanip>

// ─────────────────────────────────────────────
//  Shared utilities
// ─────────────────────────────────────────────

// Tokenizes a whitespace-separated args string
// Respects quoted strings as single tokens
static std::vector<std::string> tokenize(const std::string& args_str) {
    std::vector<std::string> tokens;
    std::istringstream ss(args_str);
    std::string token;

    while (ss >> std::ws) {
        if (ss.peek() == '"') {
            // quoted string — read until closing quote
            std::string quoted;
            ss >> std::quoted(quoted);
            tokens.push_back(quoted);
        } else {
            ss >> token;
            tokens.push_back(token);
        }
    }

    return tokens;
}

static bool contains_variable_ref(const std::string& s) {
    return s.find("${") != std::string::npos;
}

static VariableValue make_value(const std::string& raw, const std::string& file_path, int line) {
    VariableValue v;
    v.raw_value  = raw;
    v.file_path  = file_path;
    v.line       = line;
    return v;
}

// Gets or creates a CMakeVariable in the map
static CMakeVariable& get_or_create(
    std::unordered_map<std::string, CMakeVariable>& variable_map,
    const std::string& name,
    const std::string& file_path,
    int line
) {
    auto it = variable_map.find(name);
    if (it == variable_map.end()) {
        CMakeVariable var;
        var.name      = name;
        var.file_path = file_path;
        var.line      = line;
        variable_map[name] = var;
    }
    return variable_map[name];
}

// ─────────────────────────────────────────────
//  set()
// ─────────────────────────────────────────────

// Handles:
//   set(VAR value)
//   set(VAR val1 val2 ...)           # list form
//   set(VAR value CACHE TYPE doc)
//   set(VAR value CACHE TYPE doc FORCE)

void process_variable_set(
    const std::string& args,
    const std::string& file_path,
    int start_line,
    std::unordered_map<std::string, CMakeVariable>& variable_map
) {
    auto tokens = tokenize(args);
    if (tokens.empty()) return;

    const std::string& var_name = tokens[0];
    if (var_name.empty()) return;

    CMakeVariable& var = get_or_create(variable_map, var_name, file_path, start_line);

    // scan tokens for CACHE keyword
    bool is_cache = false;
    std::optional<std::string> cache_type;
    int value_end = static_cast<int>(tokens.size());

    for (int i = 1; i < static_cast<int>(tokens.size()); ++i) {
        if (tokens[i] == "CACHE") {
            is_cache   = true;
            value_end  = i;     // values are before CACHE keyword

            // next token is the cache type
            if (i + 1 < static_cast<int>(tokens.size())) {
                cache_type = tokens[i + 1];
            }
            break;
        }
    }

    if (is_cache) {
        var.is_cache   = true;
        var.cache_type = cache_type;
    }

    // everything between var_name and CACHE (or end) are values
    for (int i = 1; i < value_end; ++i) {
        // skip PARENT_SCOPE — it's a scope modifier, not a value
        if (tokens[i] == "PARENT_SCOPE") continue;
        var.possible_values.push_back(make_value(tokens[i], file_path, start_line));
    }
}

// ─────────────────────────────────────────────
//  list()
// ─────────────────────────────────────────────

// Handles:
//   list(APPEND VAR val1 val2 ...)
//   list(PREPEND VAR val1 val2 ...)
//   list(INSERT VAR index val1 ...)
//   list(REMOVE_ITEM VAR val1 val2 ...)

void process_variable_list(
    const std::string& args,
    const std::string& file_path,
    int start_line,
    std::unordered_map<std::string, CMakeVariable>& variable_map
) {
    auto tokens = tokenize(args);
    if (tokens.size() < 2) return;

    const std::string& operation = tokens[0];
    const std::string& var_name  = tokens[1];

    // ── APPEND / PREPEND ──────────────────────
    if (operation == "APPEND" || operation == "PREPEND") {
        CMakeVariable& var = get_or_create(variable_map, var_name, file_path, start_line);
        // tokens[2..] are the values
        for (int i = 2; i < static_cast<int>(tokens.size()); ++i) {
            var.possible_values.push_back(make_value(tokens[i], file_path, start_line));
        }
        return;
    }

    // ── INSERT ────────────────────────────────
    // list(INSERT VAR index val1 val2 ...)
    // tokens[2] is the index, tokens[3..] are values
    if (operation == "INSERT") {
        if (tokens.size() < 4) return;
        CMakeVariable& var = get_or_create(variable_map, var_name, file_path, start_line);
        // index is tokens[2], skip it
        for (int i = 3; i < static_cast<int>(tokens.size()); ++i) {
            var.possible_values.push_back(make_value(tokens[i], file_path, start_line));
        }
        return;
    }

    // ── REMOVE_ITEM ───────────────────────────
    // list(REMOVE_ITEM VAR val1 val2 ...)
    // raw string match — if value contains ${...} it still matches as-is
    if (operation == "REMOVE_ITEM") {
        auto it = variable_map.find(var_name);
        if (it == variable_map.end()) return;   // variable doesn't exist, nothing to remove

        CMakeVariable& var = it->second;

        for (int i = 2; i < static_cast<int>(tokens.size()); ++i) {
            const std::string& to_remove = tokens[i];
            var.possible_values.erase(
                std::remove_if(
                    var.possible_values.begin(),
                    var.possible_values.end(),
                    [&](const VariableValue& v) {
                        return v.raw_value == to_remove;
                    }
                ),
                var.possible_values.end()
            );
        }
        return;
    }

    // all other list operations (FILTER etc.) are ignored
}

// ─────────────────────────────────────────────
//  option()
// ─────────────────────────────────────────────

// Handles:
//   option(FOO "description" ON)
//   option(FOO "description" OFF)
//
// Folds into CMakeVariable with is_option = true
// Both ON and OFF always added to possible_values
// Default stored in default_value

void process_option(
    const std::string& args,
    const std::string& file_path,
    int start_line,
    std::unordered_map<std::string, CMakeVariable>& variable_map
) {
    auto tokens = tokenize(args);
    // minimum: VAR "description" DEFAULT
    if (tokens.size() < 3) return;

    const std::string& var_name     = tokens[0];
    // tokens[1] is the docstring — we skip it
    const std::string& default_val  = tokens[2];

    CMakeVariable& var = get_or_create(variable_map, var_name, file_path, start_line);

    var.is_option     = true;
    var.default_value = default_val;

    // both ON and OFF are always possible regardless of default
    var.possible_values.push_back(make_value("ON",  file_path, start_line));
    var.possible_values.push_back(make_value("OFF", file_path, start_line));
}